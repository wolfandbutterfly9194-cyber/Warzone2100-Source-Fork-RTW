#include <chrono>
#include <cstdio>
#include <SDL3/SDL_events.h>
#include "usa_metagame_bridge.h"
#include "usa_overworld_ui.h"
#include "usa_battle_setup.h"
#include "usa_campaign_state.h"
#include "template.h"
#include "design.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/framework/frameresource.h"
#include "intimage.h"
#include "component.h"
#include "init.h"
#include "levels.h"
#include "structure.h"
#include "research.h"
#include "lib/widget/widget.h"
#include <physfs.h>
#include "hci.h"
#include "frontend.h"
#include "usa_overworld_ui.h"
#include "usa_battle_setup.h"

#include "lib/framework/frame.h"
#include "lib/framework/input.h"
#include "lib/framework/physfs_ext.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/widget/widget.h"
#include "lib/widget/form.h"
#include "lib/widget/label.h"
#include "lib/widget/button.h"

#include "hci.h"
#include "frontend.h"
#include "advvis.h"
#include "visibility.h"
#include "qtscript.h"
#include "keybind.h"
extern bool godMode;
extern bool radarPermitted;
#include "objmem.h"
#include "structure.h"
#include "droid.h"
#include "map.h"
#include "order.h"
#include "power.h"
#include <sstream>
#include "intdisplay.h"
#include "multiplay.h"
#include "multiint.h"
#include "frontend.h"
#include "main.h"
#include "titleui/titleui.h"
#include "clparse.h"

#include <nlohmann/json.hpp>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

bool scavRallyPending = false;
int scavWaveNumber = 1;
std::chrono::steady_clock::time_point scavRallyStartTime;


void dispatchOpforOrders(bool stageAtRally);

void usaProduceOpforUnits(bool isScav);
bool bUsaPrebattle = false;
bool bUsaStripVanillaLobby = (USA_RESTORE_VANILLA_SKIRMISH == 0);
bool bUsaSessionActive = false;
bool bUsaGeneralDesignMode = false;
UsaOverworldDesignMode usaOverworldDesignMode = UsaOverworldDesignMode::NONE;

namespace
{
    bool overworldDesignStatsLoaded = false;

    bool usaEnsureOverworldDesignAssets()
    {
        if (overworldDesignStatsLoaded) return true;
        rebuildSearchPath(mod_multiplay, true);
        resLoad("wrf/vidmem3.wrf", 1);
        resLoad("wrf/basic.wrf", 2);
        resLoad("wrf/stats.wrf", 3);
        resLoad("wrf/camres.wrf", 4);
        if (IntImages == nullptr) imageInitBitmaps();
        auto allocList = [](COMPONENT_TYPE t, size_t n) {
            if (apCompLists[0][t] == nullptr && n > 0)
            {
                apCompLists[0][t] = new UBYTE[n];
                std::memset(apCompLists[0][t], AVAILABLE, n);
            }
        };
        allocList(COMP_BODY, asBodyStats.size());
        allocList(COMP_PROPULSION, asPropulsionStats.size());
        allocList(COMP_WEAPON, asWeaponStats.size());
        allocList(COMP_SENSOR, asSensorStats.size());
        allocList(COMP_ECM, asECMStats.size());
        allocList(COMP_CONSTRUCT, asConstructStats.size());
        allocList(COMP_REPAIRUNIT, asRepairStats.size());
        allocList(COMP_BRAIN, asBrainStats.size());
        selectedPlayer = 0;
        overworldDesignStatsLoaded = !asBodyStats.empty();
        return overworldDesignStatsLoaded;
    }
}

void usaOpenOverworldDesigner(UsaOverworldDesignMode mode)
{
    if (!usaEnsureOverworldDesignAssets()) return;
    if (usaOverworldDesignMode != UsaOverworldDesignMode::NONE)
    {
        intRemoveDesign();
    }
    usaOverworldDesignMode = mode;
    bUsaGeneralDesignMode = (mode == UsaOverworldDesignMode::GENERAL_DESIGN);
    bMultiPlayer = true;
    intAddDesign(true);
    intProcessDesign(IDDES_TEMPLSTART);
    widgReveal(psWScreen, IDDES_STOREBUTTON); widgSetButtonState(psWScreen, IDDES_STOREBUTTON, 0);
    widgReveal(psWScreen, IDDES_FORM);
}

void usaCloseOverworldDesigner()
{
    if (usaOverworldDesignMode != UsaOverworldDesignMode::NONE)
    {
        usaOverworldDesignMode = UsaOverworldDesignMode::NONE;
        bUsaGeneralDesignMode = false;
        intRemoveDesign();
    }
}
bool bUsaExitDesignToMap = false;

namespace
{
	constexpr UDWORD ID_USA_PANEL = 91000;
	constexpr UDWORD ID_USA_BTN_LAUNCH = 91001;
	constexpr UDWORD ID_USA_BTN_ABORT = 91002;
	constexpr UDWORD ID_USA_INGAME_SAVEEXIT = 91005;

	bool screenInitialized = false;
	bool inGameHudAttached = false;
	bool isBaseBuildMode = false;

	struct BriefingData
	{
		std::string stateName = "UNKNOWN SECTOR";
		std::string factionName = "HOSTILE FORCES";
		std::string mapName = "Sk-Rush";
		std::string mode = "battle";
		int turn = 1;
		int power = 1500;
		std::vector<std::string> techs;
		std::vector<std::string> squad;
	};

	BriefingData loadBriefingJson()
	{
		BriefingData data;
		if (!PHYSFS_exists("tests/usa_briefing.json")) return data;

		PHYSFS_File* file = PHYSFS_openRead("tests/usa_briefing.json");
		if (!file) return data;

		PHYSFS_sint64 len = PHYSFS_fileLength(file);
		std::string rawJson;
		if (len > 0)
		{
			rawJson.resize(static_cast<size_t>(len));
			PHYSFS_readBytes(file, rawJson.data(), static_cast<PHYSFS_uint64>(len));
		}
		PHYSFS_close(file);

		if (rawJson.empty()) return data;

		try
		{
			auto j = nlohmann::json::parse(rawJson);
			data.stateName = j.value("stateName", data.stateName);
			data.factionName = j.value("factionName", data.factionName);
			data.mapName = j.value("mapName", data.mapName);
			data.mode = j.value("mode", data.mode);
			data.turn = j.value("turn", 1);
			data.power = j.value("power", 1500);

			if (j.contains("techs") && j["techs"].is_array())
			{
				for (const auto& t : j["techs"]) data.techs.push_back(t.get<std::string>());
			}
			if (j.contains("squad") && j["squad"].is_array())
			{
				for (const auto& s : j["squad"]) data.squad.push_back(s.get<std::string>());
			}
		}
		catch (...)
		{
			debug(LOG_WARNING, "USA Prebattle: Failed to parse tests/usa_briefing.json");
		}
		return data;
	}

	void makeBriefingLabel(const std::shared_ptr<WIDGET>& parent, int x, int y, int w, int h, const std::string& text, WzTextAlignment align = WLAB_ALIGNLEFT, PIELIGHT color = WZCOL_TEXT_BRIGHT)
	{
		auto lbl = std::make_shared<W_LABEL>();
		parent->attach(lbl);
		lbl->setGeometry(x, y, w, h);
		lbl->setString(WzString::fromUtf8(text));
		lbl->setTextAlignment(align);
		lbl->setFont(font_regular, color);
	}

	void makeStyledButton(UDWORD parentFormId, UDWORD id, int x, int y, int w, int h, const std::string& label, bool disabled = false)
	{
		W_BUTINIT sButInit;
		sButInit.formID = parentFormId;
		sButInit.id = id;
		sButInit.x = static_cast<short>(x);
		sButInit.y = static_cast<short>(y);
		sButInit.width = static_cast<uint16_t>(w);
		sButInit.height = static_cast<uint16_t>(h);
		sButInit.style = WBUT_TXTCENTRE | (disabled ? WBUT_DISABLE : 0);
		sButInit.FontID = font_medium;
		sButInit.pText = label.c_str();
		sButInit.pDisplay = displayTextOption;
		sButInit.pUserData = new DisplayTextOptionCache();
		sButInit.onDelete = [](WIDGET* psWidget) {
			delete static_cast<DisplayTextOptionCache*>(psWidget->pUserData);
			psWidget->pUserData = nullptr;
			};

		widgAddButton(psWScreen, &sButInit);
	}
}
void startUsaPrebattleScreen()
{
    if (screenInitialized) return;
    widgDelete(psWScreen, FRONTEND_BOTFORM);
    widgDelete(psWScreen, FRONTEND_TOPFORM);
    usaInitOverworldUi();
    screenInitialized = true;
}

void runUsaPrebattleTick()
{
    if (!screenInitialized) startUsaPrebattleScreen();
    if (usaOverworldDesignMode != UsaOverworldDesignMode::NONE)
    {
        if (keyPressed(KEY_ESC))
        {
            usaCloseOverworldDesigner();
            renderUsaOverworldUi();
            return;
        }
        pie_BoxFill(0, 0, static_cast<int>(screenWidth), static_cast<int>(screenHeight), pal_RGBA(14, 20, 30, 255));
        WidgetTriggers const &triggers = widgRunScreen(psWScreen);
        for (const auto &trigger : triggers)
        {
            if (trigger.widget != nullptr)
            {
                intProcessDesign(trigger.widget->id);
            }
        }
        intRunDesign();
        widgDisplayScreen(psWScreen);
        return;
    }
    if (!runUsaOverworldUi()) { screenInitialized = false; return; }
    renderUsaOverworldUi();
}
void saveUsaBaseSnapshotAndExit()
{
    std::ostringstream out;
    out << "USA_SNAPSHOT_BEGIN POWER=" << static_cast<int>(getPower(0)) << "\n";

    for (const auto& psStruct : apsStructLists.at(0))
    {
        if (psStruct != nullptr && !psStruct->died && psStruct->status == SS_BUILT && psStruct->pStructureType != nullptr)
        {
            int tx = psStruct->pos.x / 128;
            int ty = psStruct->pos.y / 128;
            out << "USA_STRUCT|" << psStruct->pStructureType->id.toUtf8() << " " << tx << " " << ty << "\n";
        }
    }

    for (const auto& psDroid : apsDroidLists.at(0))
    {
        if (psDroid != nullptr && !psDroid->died)
        {
            const auto* bodyStats = psDroid->getBodyStats();
            const auto* propStats = psDroid->getPropulsionStats();
            const auto* weapStats = (psDroid->numWeaps > 0) ? psDroid->getWeaponStats(0) : nullptr;

            std::string bodyId = bodyStats ? bodyStats->id.toUtf8() : "Body1REC";
            std::string propId = propStats ? propStats->id.toUtf8() : "wheeled01";
            std::string weapId = weapStats ? weapStats->id.toUtf8() : "Spade1Mk1";

            int tx = psDroid->pos.x / 128;
            int ty = psDroid->pos.y / 128;
            out << "USA_DROID|" << bodyId << " " << propId << " " << weapId << " " << tx << " " << ty << "\n";
        }
    }

    std::string payload = out.str();
    PHYSFS_File* f = PHYSFS_openWrite("tests/usa_base_snapshot.txt");
    if (f != nullptr)
    {
        PHYSFS_writeBytes(f, payload.data(), payload.size());
        PHYSFS_close(f);
    }

    usaExportSessionResults(true);
    usaExportSessionResults(true);
    debug(LOG_INFO, "USA_HUD_SAVE_EXIT_TRIGGERED");
    fflush(nullptr);
    bUsaSessionActive = false;
    bUsaPrebattle = true;
    changeTitleMode(TITLE);
}

static std::chrono::steady_clock::time_point combatStartTime;
static bool combatTimerStarted = false;
static bool opforTriggeredNow = false;
static int lastShownSec = -1;

void triggerOpforAttackNow()
{
    

    

    if (opforTriggeredNow)
    {
        return;
    }
    opforTriggeredNow = true;
    triggerEventChat(0, -1, "USA_ATTACK_NOW"); PHYSFS_File* f = PHYSFS_openWrite("challenges/usa_opfor_trigger.flag");
    if (f != nullptr)
    {
        const char* msg = "opforCountdownSec = 0;\n";
        PHYSFS_writeBytes(f, msg, 23);
        PHYSFS_close(f);
    }
    WIDGET* statusBtn = widgGetFromID(psWScreen, ID_USA_INGAME_SAVEEXIT);
    if (statusBtn != nullptr)
    {
        static_cast<W_BUTTON*>(statusBtn)->setString("OPFOR AWARE // HOSTILES ADVANCING");
    }
    WIDGET* atkBtn = widgGetFromID(psWScreen, ID_USA_INGAME_SAVEEXIT + 1);
    if (atkBtn != nullptr)
    {
        widgSetButtonState(psWScreen, ID_USA_INGAME_SAVEEXIT + 1, WBUT_DISABLE);
    }
}

void usaProduceOpforUnits(bool isScav)
{
    static int waveRound = 0;
    waveRound++;

    struct ScavDef { const char* name; const char* body; const char* prop; const char* weap; };
    static const ScavDef scavDefs[] = {
        { "Scav Raider",     "B1BaBaPerson01",     "BaBaLegs", "BaBaMG"     },
        { "Scav Trike",      "B4body-sml-trike01", "BaBaProp", "Rocket-Pod" },
        { "Scav Buggy",      "B3body-sml-buggy01", "BaBaProp", "Flame1Mk1"  },
        { "Scav Battle Bus", "B2JeepBody",         "BaBaProp", "Cannon1Mk1" },
        { "Scav Firetruck",  "B2tractor",          "BaBaProp", "Flame1Mk1"  }
    };

    std::vector<DROID_TEMPLATE*> scavTemplates;
    if (isScav)
    {
        for (const auto& sd : scavDefs)
        {
            DROID_TEMPLATE* found = nullptr;
            enumerateTemplates(1, [&](DROID_TEMPLATE* psT) {
                if (psT && psT->name.toUtf8() == sd.name) { found = psT; return false; }
                return true;
            });
            if (found == nullptr)
            {
                int bIdx = getCompFromID(COMP_BODY, WzString::fromUtf8(sd.body));
                int pIdx = getCompFromID(COMP_PROPULSION, WzString::fromUtf8(sd.prop));
                int wIdx = getCompFromID(COMP_WEAPON, WzString::fromUtf8(sd.weap));
                if (bIdx < 0) bIdx = getCompFromID(COMP_BODY, WzString::fromUtf8("Body1REC"));
                if (pIdx < 0) pIdx = getCompFromID(COMP_PROPULSION, WzString::fromUtf8("wheeled01"));
                if (wIdx < 0) wIdx = getCompFromID(COMP_WEAPON, WzString::fromUtf8("MG1Mk1"));
                if (bIdx >= 0 && pIdx >= 0)
                {
                    auto tmpl = std::make_unique<DROID_TEMPLATE>();
                    tmpl->name = WzString::fromUtf8(sd.name);
                    tmpl->id = WzString::fromUtf8(sd.name);
                    for (int c = 0; c < DROID_MAXCOMP; ++c) tmpl->asParts[c] = 0;
                    tmpl->asParts[COMP_BODY] = bIdx;
                    tmpl->asParts[COMP_PROPULSION] = pIdx;
                    if (wIdx >= 0) { tmpl->numWeaps = 1; *(tmpl->asWeaps) = wIdx; }
                    tmpl->droidType = droidTemplateType(tmpl.get());
                    found = addTemplate(1, std::move(tmpl));
                }
            }
            if (found != nullptr) scavTemplates.push_back(found);
        }
    }

    int fIdx = 0;
    for (STRUCTURE* psStruct : apsStructLists.at(1))
    {
        if (!psStruct || psStruct->died || psStruct->status != SS_BUILT) continue;
        if (!psStruct->isFactory() && psStruct->pStructureType->id.toUtf8() != "A0BaBaFactory") continue;

        if (!isScav)
        {
            if (psStruct->pFunctionality->factory.psSubject != nullptr) continue;
            std::vector<DROID_TEMPLATE*> avail = fillTemplateList(psStruct);
            std::vector<DROID_TEMPLATE*> combat;
            for (DROID_TEMPLATE* t : avail)
            {
                if (t && t->droidType != DROID_CONSTRUCT && t->droidType != DROID_TRANSPORTER && t->droidType != DROID_SUPERTRANSPORTER)
                {
                    combat.push_back(t);
                }
            }
            if (!combat.empty())
            {
                structSetManufacture(psStruct, combat.at((fIdx + waveRound) % combat.size()), ModeImmediate);
            }
        }
        else if (!scavTemplates.empty())
        {
            size_t waveCap = std::min<size_t>(110, 35 + static_cast<size_t>(scavWaveNumber) * 15); if (apsDroidLists.at(1).size() >= waveCap) break;
            DROID_TEMPLATE* psChosen = scavTemplates[(fIdx + waveRound) % scavTemplates.size()];
            int spawnX = std::max(world_coord(4), std::min(world_coord(mapWidth - 5), psStruct->pos.x + world_coord((fIdx % 3) - 1)));
            int spawnY = std::max(world_coord(4), std::min(world_coord(mapHeight - 5), psStruct->pos.y + world_coord(4)));
            DROID* psNew = reallyBuildDroid(psChosen, Position(spawnX, spawnY, 0), 1, false);
            if (psNew != nullptr) { addDroid(psNew, apsDroidLists); }
        }
        fIdx++;
    }
}
void dispatchOpforOrders(bool stageAtRally)
{
    std::vector<BASE_OBJECT*> p0Targets;
    for (STRUCTURE* psStruct : apsStructLists.at(0))
    {
        if (psStruct && !psStruct->died) p0Targets.push_back(psStruct);
    }
    for (DROID* psD : apsDroidLists.at(0))
    {
        if (psD && !psD->died) p0Targets.push_back(psD);
    }
    if (p0Targets.empty()) return;

    int baseX = p0Targets.front()->pos.x;
    int baseY = p0Targets.front()->pos.y;

    if (stageAtRally)
    {
        int mapMidX = world_coord(mapWidth / 2);
        int mapMidY = world_coord(mapHeight / 2);
        double dx = static_cast<double>(mapMidX - baseX);
        double dy = static_cast<double>(mapMidY - baseY);
        double len = std::hypot(dx, dy);
        if (len < 1.0) { dx = 1.0; dy = 0.0; len = 1.0; }
        int dist = world_coord(18);
        int rallyX = std::max(world_coord(5), std::min(world_coord(mapWidth - 6), baseX + static_cast<int>((dx / len) * dist)));
        int rallyY = std::max(world_coord(5), std::min(world_coord(mapHeight - 6), baseY + static_cast<int>((dy / len) * dist)));

        int idx = 0;
        for (DROID* psEnemy : apsDroidLists.at(1))
        {
            if (psEnemy && !psEnemy->died && psEnemy->droidType != DROID_CONSTRUCT)
            {
                int ox = rallyX + world_coord((idx % 5) - 2);
                int oy = rallyY + world_coord(((idx / 5) % 5) - 2);
                orderDroidLoc(psEnemy, DORDER_MOVE, ox, oy, ModeImmediate);
                idx++;
            }
        }
    }
    else
    {
        int idx = 0;
        for (DROID* psEnemy : apsDroidLists.at(1))
        {
            if (psEnemy && !psEnemy->died && psEnemy->droidType != DROID_CONSTRUCT)
            {
                BASE_OBJECT* target = p0Targets[idx % p0Targets.size()];
                orderDroidObj(psEnemy, DORDER_ATTACK, target, ModeImmediate);
                idx++;
            }
        }
    }
}

void updateUsaInGameHud()
{
        if (!bUsaSessionActive || psWScreen == nullptr)
    {
        return;
    }

    if (false && bUsaGeneralDesignMode)
    {
        static bool designOpened = false;
        if (!designOpened && psWScreen != nullptr)
        {
            designOpened = true;
            bMultiPlayer = true;
    intAddDesign(true);
    intProcessDesign(IDDES_TEMPLSTART);
    widgReveal(psWScreen, IDDES_STOREBUTTON); widgSetButtonState(psWScreen, IDDES_STOREBUTTON, 0);
    widgReveal(psWScreen, IDDES_FORM);
        }
        return;
    }

    if (!godMode)
    {
        godMode = true;
        revealAll(0);
        setRevealStatus(true);
        radarPermitted = true;
        preProcessVisibility();
    }

    if (isBaseBuildMode)
    {
        const int btnW = 320;
        const int btnH = 28;
        const int btnX = (pie_GetVideoBufferWidth() - btnW) / 2;
        const int btnY = 10;

        if (!inGameHudAttached)
        {
            makeStyledButton(
                0,
                ID_USA_INGAME_SAVEEXIT,
                btnX,
                btnY,
                btnW,
                btnH,
                "[ SAVE BASE & EXIT TO MAP (F10) ]",
                false
            );

            WIDGET* btn = widgGetFromID(psWScreen, ID_USA_INGAME_SAVEEXIT);
            if (btn != nullptr)
            {
                static_cast<W_BUTTON*>(btn)->addOnClickHandler([](W_BUTTON&) {
                    saveUsaBaseSnapshotAndExit();
                });
            }
            inGameHudAttached = true;
        }

        if (keyPressed(KEY_F10))
        {
            saveUsaBaseSnapshotAndExit();
        }
        return;
    }

    // Combat Mode: Live 4:00 OpFor Awareness Countdown + [ ATTACK NOW ] Test Button
    if (!combatTimerStarted)
    {
        combatStartTime = std::chrono::steady_clock::now();
        usaApplyNativeBattleSetup();
        combatTimerStarted = true;
        opforTriggeredNow = false;
        lastShownSec = -1;
        PHYSFS_delete("challenges/usa_opfor_trigger.flag");
    }

    const int timerW = 350;
    const int atkW = 175;
    const int gap = 16;
    const int totalW = timerW + gap + atkW;
    const int startX = (pie_GetVideoBufferWidth() - totalW) / 2;
    const int btnY = 10;
    const int btnH = 28;

    if (!inGameHudAttached)
    {
        makeStyledButton(0, ID_USA_INGAME_SAVEEXIT, startX, btnY, timerW, btnH, "OPFOR AWARE IN 4:00", true);
        makeStyledButton(0, ID_USA_INGAME_SAVEEXIT + 1, startX + timerW + gap, btnY, atkW, btnH, "[ ATTACK NOW (F9) ]", false);

        WIDGET* atkBtn = widgGetFromID(psWScreen, ID_USA_INGAME_SAVEEXIT + 1);
        if (atkBtn != nullptr)
        {
            static_cast<W_BUTTON*>(atkBtn)->addOnClickHandler([](W_BUTTON&) {
                triggerOpforAttackNow();
            });
        }
        inGameHudAttached = true;
    }

    if (keyPressed(KEY_F9))
    {
        triggerOpforAttackNow();
    }

    

    

    if (opforTriggeredNow)
    {
        bool isScav = (loadBriefingJson().factionName.find("Scav") != std::string::npos);
        {
            static int lastPostProdSec = -1;
            int postSec = static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - combatStartTime).count());
            if (postSec != lastPostProdSec && (postSec % 6 == 0))
            {
                lastPostProdSec = postSec;
                addPower(1, 2500);
                usaProduceOpforUnits(isScav);
            }
        }
        if (isScav)
        {
            auto rallyElapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - scavRallyStartTime).count();

    if (scavRallyPending)
            {
                dispatchOpforOrders(true);
                if (rallyElapsed >= 35)
                {
                    scavRallyPending = false;
                    scavRallyStartTime = std::chrono::steady_clock::now();
                    dispatchOpforOrders(false);
                    WIDGET* statusBtn = widgGetFromID(psWScreen, ID_USA_INGAME_SAVEEXIT);
                    if (statusBtn != nullptr)
                    {
                        static_cast<W_BUTTON*>(statusBtn)->setString("OPFOR AWARE // MASS ASSAULT");
                    }
                }
            }
            else if (rallyElapsed >= 25)
            {
                scavRallyPending = true;
                scavRallyStartTime = std::chrono::steady_clock::now();
                WIDGET* statusBtn = widgGetFromID(psWScreen, ID_USA_INGAME_SAVEEXIT);
                if (statusBtn != nullptr)
                {
                    static_cast<W_BUTTON*>(statusBtn)->setString("OPFOR MASSING NEXT WAVE // STAGING");
                }
            }
        }
        else
        {
            static int lastTechWaveSec = -1;
            int curSec = static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - combatStartTime).count());
            if (curSec != lastTechWaveSec && (curSec % 8 == 0))
            {
                lastTechWaveSec = curSec;
                dispatchOpforOrders(false);
            }
        }
    }
    if (!opforTriggeredNow)
    {
        for (DROID* psEnemy : apsDroidLists.at(1))
        {
            if (psEnemy && !psEnemy->died && psEnemy->droidType != DROID_CONSTRUCT && psEnemy->order.type != DORDER_HOLD)
            {
                orderDroid(psEnemy, DORDER_HOLD, ModeImmediate);
            }
        }
    }

    if (!opforTriggeredNow)
    {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - combatStartTime).count();
        int rem = 240 - static_cast<int>(elapsed);
        if (rem <= 0)
        {
            triggerOpforAttackNow();
        }
        else if (rem != lastShownSec)
        {
            lastShownSec = rem;
            int activeFacs = 0;
            std::vector<int> hudOwners = { 1 };
            int hudScavP = static_cast<int>(scavengerPlayer());
            if (hudScavP >= 0 && hudScavP < static_cast<int>(apsStructLists.size()) && hudScavP != 1) hudOwners.push_back(hudScavP);
            for (int hP : hudOwners)
            for (STRUCTURE* psS : apsStructLists.at(hP))
            {
                if (psS && !psS->died && psS->status == SS_BUILT && (psS->isFactory() || psS->pStructureType->id.toUtf8() == "A0BaBaFactory"))
                {
                    activeFacs++;
                }
            }
            if (rem % 6 == 0)
            {
                bool scavMode = ((getUsaCampaignState().activeTargetIndex >= 0 && getUsaCampaignState().states[getUsaCampaignState().activeTargetIndex].owner == UsaFaction::SCAV_SYNDICATE) || loadBriefingJson().factionName.find("Scav") != std::string::npos);
                addPower(1, 2500);
                usaProduceOpforUnits(scavMode);
            }
            std::string minStr = std::to_string(rem / 60);
            std::string secStr = (rem % 60 < 10 ? "0" : "") + std::to_string(rem % 60);
            std::string hudStr = "OPFOR IN " + minStr + ":" + secStr + " | FACS: " + std::to_string(activeFacs) + " | ENEMIES: " + std::to_string(apsDroidLists.at(1).size());
            WIDGET* timerBtn = widgGetFromID(psWScreen, ID_USA_INGAME_SAVEEXIT);
            if (timerBtn != nullptr)
            {
                static_cast<W_BUTTON*>(timerBtn)->setString(hudStr.c_str());
            }
        }
    }
}

