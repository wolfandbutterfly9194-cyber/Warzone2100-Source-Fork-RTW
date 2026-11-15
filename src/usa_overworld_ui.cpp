#include <array>
#include "usa_overworld_ui.h"
#include "usa_campaign_state.h"
#include "usa_battle_setup.h"
#include "usa_metagame_bridge.h"

#include "lib/framework/frame.h"
#include "lib/framework/input.h"
#include "lib/framework/physfs_ext.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/bitimage.h"
#include "intimage.h"
#include "frend.h"
#include <physfs.h>
#include "lib/framework/frameresource.h"
#include "intfac.h"
#include "lib/ivis_opengl/textdraw.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/widget/widget.h"

#include "frontend.h"
#include "clparse.h"
extern std::string challengeName;
#include "main.h"
#include "multiint.h"
#include "multiplay.h"
#include "titleui/titleui.h"


#include <SDL3/SDL_timer.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <string>

namespace
{
    int activeRightTab = 0; // 0=SECTOR, 1=RESEARCH, 2=DESIGN, 3=GENERAL
    bool systemMenuOpen = false;
    bool generalDesignBayOpen = false;
    int generalDesignCategory = 0;
    int selBodyIdx = 1;
    int selPropIdx = 1;
    int selWeapIdx = 1;
    std::string overworldStatus = "Click any state on the USA map to inspect, build your base, or invade.";

    bool hitRect(int mx, int my, int x, int y, int w, int h)
    {
        return mx >= x && mx <= (x + w) && my >= y && my <= (y + h);
    }

    void drawPanelBox(int x, int y, int w, int h, PIELIGHT fill, PIELIGHT border)
    {
        pie_UniTransBoxFill(x, y, x + w, y + h, fill);
        iV_Box(x, y, x + w, y + h, border);
    }

    void drawActionBtn(int x, int y, int w, int h, const std::string& label, PIELIGHT fill, PIELIGHT border)
    {
        drawPanelBox(x, y, w, h, fill, border);
        iV_DrawText(label.c_str(), static_cast<float>(x + 10), static_cast<float>(y + (h / 2) + 4), font_regular);
    }

    void launchStateMission(int targetIdx, bool isBaseMode)
    {
        UsaCampaignState& camp = getUsaCampaignState();
        if (targetIdx < 0 || targetIdx >= static_cast<int>(camp.states.size())) return;

        camp.activeTargetIndex = targetIdx;
        camp.activeSessionIsBaseMode = isBaseMode;
        camp.activeSessionIsDefense = false;
        camp.saveToPhysfs();

        const Territory& target = camp.states[targetIdx];
        bool isScav = (!isBaseMode && target.owner == UsaFaction::SCAV_SYNDICATE);
        std::string enemyName = isBaseMode ? "Garrison Simulation" : getUsaFactionName(target.owner);
        int enemyCol = (target.owner == UsaFaction::NEW_PARADIGM) ? 1 : ((target.owner == UsaFaction::THE_COLLECTIVE) ? 5 : ((target.owner == UsaFaction::NEXUS_REMNANT) ? 6 : 7));

        std::ostringstream json;
        json << "{\n  \"challenge\": {\n"
             << "    \"name\": \"" << (isBaseMode ? "Base Ops: " : "Sector Assault: ") << target.name << "\",\n"
             << "    \"map\": \"" << target.wzMap << "\",\n"
             << "    \"maxPlayers\": 2,\n    \"scavengers\": false,\n    \"allowPositionChange\": false,\n"
             << "    \"bases\": 0,\n    \"powerLevel\": 1,\n    \"alliances\": 0,\n    \"techLevel\": 1,\n"
             << "    \"player_0\": { \"name\": \"The Project\", \"team\": 0, \"position\": 0, \"colour\": 0 },\n"
             << "    \"player_1\": { \"name\": \"" << enemyName << "\", \"team\": 1, \"position\": 1, \"colour\": " << enemyCol << ", \"ai\": \"" << ((isBaseMode || isScav) ? "null" : "cobra.json") << "\" }\n"
             << "  }\n}\n";

        PHYSFS_mkdir("tests");
        std::string jStr = json.str();
        if (PHYSFS_File* f = PHYSFS_openWrite("tests/usa.json"))
        {
            PHYSFS_writeBytes(f, jStr.data(), jStr.size());
            PHYSFS_close(f);
        }

        std::ostringstream brief;
        brief << "{\n  \"stateName\": \"" << target.name << " (" << target.code << ")\",\n"
              << "  \"factionName\": \"" << enemyName << "\",\n"
              << "  \"mapName\": \"" << target.wzMap << "\",\n"
              << "  \"mode\": \"" << (isBaseMode ? "base" : "battle") << "\",\n"
              << "  \"turn\": " << camp.turn << ",\n  \"power\": " << camp.power << "\n}\n";
        std::string bStr = brief.str();
        if (PHYSFS_File* bf = PHYSFS_openWrite("tests/usa_briefing.json"))
        {
            PHYSFS_writeBytes(bf, bStr.data(), bStr.size());
            PHYSFS_close(bf);
        }

        bUsaPrebattle = false;
        bUsaStripVanillaLobby = (USA_RESTORE_VANILLA_SKIRMISH == 0);
        bUsaSessionActive = true;

        usaSetSkirmishTestName("usa");
        challengeName = "usa";
        setHostLaunch(HostLaunch::Skirmish);
        SPinit(LEVEL_TYPE::SKIRMISH);
        bMultiPlayer = true;
        ingame.side = InGameSide::HOST_OR_SINGLEPLAYER;
        game.type = LEVEL_TYPE::SKIRMISH;
        changeTitleMode(TITLE);
        changeTitleUI(std::make_shared<WzMultiplayerOptionsTitleUI>(wzTitleUICurrent));
    }
}
void usaInitOverworldUi()
{
    UsaCampaignState& camp = getUsaCampaignState();
    if (camp.selectedIndex < 0 || camp.selectedIndex >= static_cast<int>(camp.states.size()))
    {
        camp.selectedIndex = 2; // CA
    }
    if (!PHYSFS_exists("tests/usa.json"))
    {
        PHYSFS_mkdir("tests");
        std::string defJson = "{\n  \"challenge\": {\n    \"name\": \"Sector Assault: Oregon\",\n    \"map\": \"Sk-Mountain\",\n    \"maxPlayers\": 2,\n    \"scavengers\": false,\n    \"allowPositionChange\": false,\n    \"bases\": 0,\n    \"powerLevel\": 1,\n    \"alliances\": 0,\n    \"techLevel\": 1,\n    \"player_0\": { \"name\": \"The Project\", \"team\": 0, \"position\": 0, \"colour\": 0 },\n    \"player_1\": { \"name\": \"Western Scav Syndicate\", \"team\": 1, \"position\": 1, \"colour\": 7, \"ai\": \"null\" }\n  }\n}\n";
        if (PHYSFS_File* f = PHYSFS_openWrite("tests/usa.json"))
        {
            PHYSFS_writeBytes(f, defJson.data(), defJson.size());
            PHYSFS_close(f);
        }
    }
}

bool runUsaOverworldUi()
{
    UsaCampaignState& camp = getUsaCampaignState();
    const int sw = std::max(1024, static_cast<int>(screenWidth));
    const int sh = std::max(700, static_cast<int>(screenHeight));
    const float sx = static_cast<float>(sw) / 1360.0f;
    const float sy = static_cast<float>(sh) / 780.0f;
    const int mx = mouseX(), my = mouseY();

    if (keyPressed(KEY_ESC))
    {
        if (generalDesignBayOpen) { generalDesignBayOpen = false; return true; }
        systemMenuOpen = !systemMenuOpen;
        return true;
    }

    if (mousePressed(MOUSE_LMB))
    {
        if (hitRect(mx, my, static_cast<int>(1175 * sx), static_cast<int>(17 * sy), static_cast<int>(160 * sx), static_cast<int>(34 * sy)))
        {
            generalDesignBayOpen = false;
            systemMenuOpen = !systemMenuOpen;
            return true;
        }
        if (generalDesignBayOpen)
        {
            const int dw = static_cast<int>(920 * sx), dh = static_cast<int>(540 * sy);
            const int dx0 = (sw - dw) / 2, dy0 = static_cast<int>(105 * sy);

            // Top-right [X] Close button on header bar
            if (hitRect(mx, my, dx0 + dw - static_cast<int>(44 * sx), dy0 + 6, static_cast<int>(38 * sx), static_cast<int>(34 * sy)))
            {
                generalDesignBayOpen = false;
                overworldStatus = "Closed Dragon Command General Design Bay.";
                return true;
            }

            // 4 Warzone 2100 Category Buttons: BODY, DRIVE, TURRET 1, TURRET 2
            const int catX = dx0 + static_cast<int>(20 * sx), catY = dy0 + static_cast<int>(52 * sy);
            const int catW = static_cast<int>(102 * sx), catH = static_cast<int>(38 * sy), catGap = static_cast<int>(6 * sx);
            for (int c = 0; c < 4; ++c)
            {
                if (hitRect(mx, my, catX + c * (catW + catGap), catY, catW, catH))
                {
                    generalDesignCategory = c;
                    return true;
                }
            }

            // Clickable Part Cards List
            std::vector<GeneralPartOption>* activeList = &camp.general.bodyOptions;
            std::string* activeTargetId = &camp.general.bodyId;
            if (generalDesignCategory == 1) { activeList = &camp.general.propOptions; activeTargetId = &camp.general.propId; }
            else if (generalDesignCategory == 2) { activeList = &camp.general.weap1Options; activeTargetId = &camp.general.weapon1Id; }
            else if (generalDesignCategory == 3) { activeList = &camp.general.weap2Options; activeTargetId = &camp.general.weapon2Id; }

            const int cardX = dx0 + static_cast<int>(20 * sx), cardW = static_cast<int>(426 * sx), cardH = static_cast<int>(54 * sy);
            for (int idx = 0; idx < static_cast<int>(activeList->size()); ++idx)
            {
                int cardY = dy0 + static_cast<int>((102 + idx * 62) * sy);
                if (hitRect(mx, my, cardX, cardY, cardW, cardH))
                {
                    *activeTargetId = (*activeList)[idx].id;
                    overworldStatus = "Equipped on General: " + (*activeList)[idx].name;
                    return true;
                }
            }

            // Bottom-Right [ SAVE & LOCK DESIGN ] and [ CLOSE BAY ] buttons
            const int rx = dx0 + static_cast<int>(466 * sx), rw = static_cast<int>(432 * sx);
            const int btnW = (rw - static_cast<int>(12 * sx)) / 2;
            const int btnY = dy0 + dh - static_cast<int>(60 * sy), btnH = static_cast<int>(44 * sy);
            if (hitRect(mx, my, rx, btnY, btnW, btnH))
            {
                camp.saveToPhysfs();
                generalDesignBayOpen = false;
                overworldStatus = "Dragon Command General design saved & locked.";
                return true;
            }
            if (hitRect(mx, my, rx + btnW + static_cast<int>(12 * sx), btnY, btnW, btnH))
            {
                generalDesignBayOpen = false;
                overworldStatus = "Exited Dragon Command General Design Bay.";
                return true;
            }
            return true;
        }
        if (systemMenuOpen)
        {
            const int mw = static_cast<int>(380 * sx), mx0 = (sw - mw) / 2, bw = mw - static_cast<int>(40 * sx), bx0 = mx0 + static_cast<int>(20 * sx), bh = static_cast<int>(40 * sy);
            if (hitRect(mx, my, bx0, static_cast<int>(220 * sy), bw, bh)) { camp.saveToPhysfs(); systemMenuOpen = false; overworldStatus = "Campaign saved to usa_campaign_save.txt."; return true; }
            if (hitRect(mx, my, bx0, static_cast<int>(270 * sy), bw, bh)) { camp.loadFromPhysfs(); systemMenuOpen = false; overworldStatus = "Campaign loaded from usa_campaign_save.txt."; return true; }
            if (hitRect(mx, my, bx0, static_cast<int>(320 * sy), bw, bh)) { systemMenuOpen = false; usaOpenOptionsMenu(); return true; }
            if (hitRect(mx, my, bx0, static_cast<int>(370 * sy), bw, bh)) { camp.initDefault(); camp.saveToPhysfs(); systemMenuOpen = false; overworldStatus = "Started fresh Global Reclamation Campaign."; return true; }
            if (hitRect(mx, my, bx0, static_cast<int>(420 * sy), bw, bh)) { systemMenuOpen = false; bUsaPrebattle = false; startTitleMenu(); return false; }
            if (hitRect(mx, my, bx0, static_cast<int>(480 * sy), bw, bh)) { systemMenuOpen = false; return true; }
            return true;
        }
        const float mapScaleY = sy * 1.24f, mapOffY = -20.0f * sy;
        const UsaVec2 mapPt{ static_cast<float>(mx) / sx, (static_cast<float>(my) - mapOffY) / mapScaleY };
        for (int i = 0; i < static_cast<int>(camp.states.size()); ++i)
        {
            if (usaPointInPolygon(mapPt, camp.states[i].polygon))
            {
                camp.selectedIndex = i;
                activeRightTab = 0;
                overworldStatus = "Selected " + camp.states[i].name + " (" + camp.states[i].code + ")";
                return true;
            }
        }

        const int tabStartX = static_cast<int>(910 * sx), tabY = static_cast<int>(65 * sy), tabW = static_cast<int>(105 * sx), tabH = static_cast<int>(36 * sy), tabGap = static_cast<int>(5 * sx);
        std::array<const char*, 4> tabNames = {{ "SECTOR", "RESEARCH", "DESIGN", "GENERAL" }};
        for (int tb = 0; tb < 4; ++tb)
        {
            if (hitRect(mx, my, tabStartX + tb * (tabW + tabGap), tabY, tabW, tabH)) { activeRightTab = tb; overworldStatus = std::string("Console Tab: ") + tabNames[tb]; if (tb == 2) usaOpenOverworldDesigner(UsaOverworldDesignMode::CAMPAIGN_DESIGN); return true; }
        }

        const int bx = static_cast<int>(922 * sx), bw = static_cast<int>(411 * sx);

        // Dedicated END TURN Button (always visible at bottom of right console)
        if (hitRect(mx, my, bx, static_cast<int>(666 * sy), bw, static_cast<int>(42 * sy)))
        {
            int inc = 0; for (const auto& s : camp.states) if (s.owner == UsaFaction::THE_PROJECT) inc += s.powerIncome;
            camp.turn++; camp.power += inc; camp.saveToPhysfs();
            overworldStatus = "Turn " + std::to_string(camp.turn) + ": Collected +" + std::to_string(inc) + " PWR across controlled territories.";
            return true;
        }

        if (activeRightTab == 0 && hitRect(mx, my, bx, static_cast<int>(265 * sy), bw, static_cast<int>(44 * sy)))
        {
            const Territory& sel = camp.states[camp.selectedIndex];
            if (sel.owner == UsaFaction::THE_PROJECT) { launchStateMission(camp.selectedIndex, true); return false; }
            bool adj = false;
            for (const std::string& nb : sel.neighbors)
                for (const Territory& st : camp.states)
                    if (st.code == nb && st.owner == UsaFaction::THE_PROJECT) adj = true;
            if (adj) { launchStateMission(camp.selectedIndex, false); return false; }
            overworldStatus = "[!] Must border an owned state to invade " + sel.name;
            return true;
        }
        else if (activeRightTab == 1)
        {
            for (int t = 0; t < static_cast<int>(camp.techs.size()); ++t)
            {
                if (hitRect(mx, my, bx, static_cast<int>((150 + t * 92) * sy), bw, static_cast<int>(78 * sy)))
                {
                    TechUpgrade& tech = camp.techs[t];
                    if (!tech.unlocked && camp.power >= tech.cost) { camp.power -= tech.cost; tech.unlocked = true; camp.saveToPhysfs(); overworldStatus = "Unlocked: " + tech.label; }
                    return true;
                }
            }
        }
        else if (activeRightTab == 2)
        {
            int pBtnW = static_cast<int>(40 * sx), rowH = static_cast<int>(32 * sy);
            if (hitRect(mx, my, bx, static_cast<int>(158 * sy), pBtnW, rowH)) selBodyIdx = (selBodyIdx + static_cast<int>(camp.bodies.size()) - 1) % static_cast<int>(camp.bodies.size());
            if (hitRect(mx, my, bx + bw - pBtnW, static_cast<int>(158 * sy), pBtnW, rowH)) selBodyIdx = (selBodyIdx + 1) % static_cast<int>(camp.bodies.size());
            if (hitRect(mx, my, bx, static_cast<int>(198 * sy), pBtnW, rowH)) selPropIdx = (selPropIdx + static_cast<int>(camp.props.size()) - 1) % static_cast<int>(camp.props.size());
            if (hitRect(mx, my, bx + bw - pBtnW, static_cast<int>(198 * sy), pBtnW, rowH)) selPropIdx = (selPropIdx + 1) % static_cast<int>(camp.props.size());
            if (hitRect(mx, my, bx, static_cast<int>(238 * sy), pBtnW, rowH)) selWeapIdx = (selWeapIdx + static_cast<int>(camp.weapons.size()) - 1) % static_cast<int>(camp.weapons.size());
            if (hitRect(mx, my, bx + bw - pBtnW, static_cast<int>(238 * sy), pBtnW, rowH)) selWeapIdx = (selWeapIdx + 1) % static_cast<int>(camp.weapons.size());

            if (hitRect(mx, my, bx, static_cast<int>(290 * sy), bw, static_cast<int>(38 * sy)))
            {
                const auto& b = camp.bodies[selBodyIdx]; const auto& p = camp.props[selPropIdx]; const auto& w = camp.weapons[selWeapIdx];
                int cost = b.cost + p.cost + w.cost;
                if (camp.power >= cost)
                {
                    camp.power -= cost;
                    std::string bpName = b.name + " " + w.name;
                    bool merged = false;
                    for (auto& ex : camp.squad) if (ex.bodyId == b.id && ex.propId == p.id && ex.weaponId == w.id) { ex.count++; merged = true; break; }
                    if (!merged) camp.squad.push_back({ bpName, b.id, p.id, w.id, cost, 1 });
                    camp.saveToPhysfs();
                    overworldStatus = "Assembled " + bpName + " (-" + std::to_string(cost) + " PWR).";
                }
                return true;
            }
        }
        if (activeRightTab == 3)
        {
            for (int v = 0; v < std::min<int>(5, static_cast<int>(camp.general.variants.size())); ++v)
            {
                int vy = static_cast<int>((462 + v * 36) * sy);
                if (hitRect(mx, my, bx, vy, bw, static_cast<int>(32 * sy)))
                {
                    const auto& var = camp.general.variants[v];
                    camp.general.bodyId = var.bodyId; camp.general.propId = var.propId; camp.general.weapon1Id = var.weapon1Id; camp.general.weapon2Id = var.weapon2Id;
                    camp.saveToPhysfs();
                    overworldStatus = "Equipped General Variant: " + var.name + " [" + var.weapon1Id + " + " + var.weapon2Id + "]";
                    return true;
                }
            }
            if (hitRect(mx, my, bx, static_cast<int>(254 * sy), bw, static_cast<int>(38 * sy)))
            {
                usaOpenOverworldDesigner(UsaOverworldDesignMode::GENERAL_DESIGN);
                overworldStatus = "Opened Dragon Command General 3D Designer (ESC or Store to return).";
                return true;
            }
            for (int g = 0; g < static_cast<int>(camp.general.upgrades.size()); ++g)
            {
                int gy = static_cast<int>((304 + g * 58) * sy);
                int gh = static_cast<int>(48 * sy);
                if (hitRect(mx, my, bx, gy, bw, gh))
                {
                    auto& node = camp.general.upgrades[g];
                    int cost = node.baseCost + node.level * 120;
                    if (node.level < node.maxLevel && camp.power >= cost)
                    {
                        camp.power -= cost;
                        node.level++;
                        camp.saveToPhysfs();
                        overworldStatus = "General Upgraded: " + node.label + " -> LVL " + std::to_string(node.level);
                    }
                    else if (node.level >= node.maxLevel)
                    {
                        overworldStatus = node.label + " is already MAX LEVEL (5/5).";
                    }
                    else
                    {
                        overworldStatus = "[!] Need " + std::to_string(cost) + " PWR for " + node.label;
                    }
                    return true;
                }
            }
        }
    }
    return true;
}

namespace
{
    void fillUsaPolygon(const std::vector<UsaVec2>& poly, float sx, float sy, PIELIGHT color)
    {
        if (poly.size() < 3) return;
        float minY = 99999.0f, maxY = -99999.0f;
        for (const auto& v : poly) { minY = std::min(minY, v.y * sy); maxY = std::max(maxY, v.y * sy); }
        int yStart = static_cast<int>(std::ceil(minY));
        int yEnd = static_cast<int>(std::floor(maxY));
        std::vector<float> nodes;
        nodes.reserve(poly.size());

        for (int y = yStart; y <= yEnd; ++y)
        {
            nodes.clear();
            float fy = static_cast<float>(y) + 0.5f;
            size_t j = poly.size() - 1;
            for (size_t i = 0; i < poly.size(); ++i)
            {
                float yi = poly[i].y * sy, yj = poly[j].y * sy;
                float xi = poly[i].x * sx, xj = poly[j].x * sx;
                if ((yi < fy && yj >= fy) || (yj < fy && yi >= fy))
                {
                    nodes.push_back(xi + (fy - yi) / (yj - yi) * (xj - xi));
                }
                j = i;
            }
            std::sort(nodes.begin(), nodes.end());
            for (size_t k = 0; k + 1 < nodes.size(); k += 2)
            {
                int x0 = static_cast<int>(std::ceil(nodes[k]));
                int x1 = static_cast<int>(std::floor(nodes[k + 1]));
                if (x1 >= x0) pie_BoxFill(x0, y, x1 + 1, y + 1, color);
            }
        }
    }

    void drawWzPng(const char* imgName, int x, int y, int w, int h)
    {
        static bool atlasLoaded = false;
        if (!atlasLoaded)
        {
            atlasLoaded = true;
            PHYSFS_mount("C:/Games/warzone2100_source/data/base", nullptr, 1);
            PHYSFS_mount("../../data/base", nullptr, 1);
            PHYSFS_mount("../data/base", nullptr, 1);
            PHYSFS_mount("data/base", nullptr, 1);
            if (IntImages == nullptr)
            {
                IntImages = iV_LoadImageFile("images/intfac.img");
            }
        }
        if (iV_GetImage(WzString::fromUtf8(imgName)) != nullptr)
        {
            iV_DrawImage2(WzString::fromUtf8(imgName), static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h));
        }
    }

    void drawWzPlate(int x, int y, int w, int h, PIELIGHT fill, PIELIGHT border, bool ticks = true)
    {
        (void)fill;
        (void)border;
        (void)ticks;
        // Authentic Warzone 2100 Frontend / Options muted slate-blue window style
        pie_UniTransBoxFill(x, y, x + w, y + h, pal_RGBA(16, 23, 34, 245));
        iV_Line(x, y, x + w, y, pal_RGBA(88, 110, 140, 225));
        iV_Line(x, y, x, y + h, pal_RGBA(78, 98, 126, 215));
        iV_Line(x, y + h, x + w, y + h, pal_RGBA(8, 11, 16, 255));
        iV_Line(x + w, y, x + w, y + h, pal_RGBA(8, 11, 16, 255));
        iV_Box(x + 1, y + 1, x + w - 1, y + h - 1, pal_RGBA(52, 68, 92, 255));
    }

    void drawWzButton(int x, int y, int w, int h, const std::string& label, PIELIGHT fill, PIELIGHT accent, bool hovered = false)
    {
        (void)fill;
        (void)accent;
        // Authentic Warzone 2100 Frontend Menu Button (muted steel-blue, zero neon glare)
        PIELIGHT bg = hovered ? pal_RGBA(34, 50, 74, 250) : pal_RGBA(22, 32, 48, 245);
        PIELIGHT edge = hovered ? pal_RGBA(125, 155, 195, 255) : pal_RGBA(60, 78, 104, 255);
        pie_UniTransBoxFill(x, y, x + w, y + h, bg);
        iV_Line(x, y, x + w, y, hovered ? pal_RGBA(145, 175, 215, 255) : pal_RGBA(86, 108, 138, 215));
        iV_Line(x, y, x, y + h, hovered ? pal_RGBA(130, 160, 200, 240) : pal_RGBA(76, 96, 124, 205));
        iV_Line(x, y + h, x + w, y + h, pal_RGBA(8, 11, 16, 255));
        iV_Line(x + w, y, x + w, y + h, pal_RGBA(8, 11, 16, 255));
        iV_Box(x + 1, y + 1, x + w - 1, y + h - 1, edge);
        drawWzPng(hovered ? "image_indicator_dot_expand.png" : "image_indicator_dot.png", x + 10, y + (h / 2) - 7, 14, 14);
        iV_DrawText(label.c_str(), static_cast<float>(x + 32), static_cast<float>(y + (h / 2) + 4), font_regular);
    }
    void drawUsaTerrainUnderMap(float sx, float sy)
    {
        // Continental terrain relief layers (Coastal Plains, Great Plains, Rockies, Cascades, Appalachians)
        // 1. Continental base landmass shelf
        fillUsaPolygon({ {95,105},{872,115},{855,175},{796,290},{750,540},{715,545},{685,475},{595,480},{465,530},{355,425},{180,415},{100,310},{100,135} },
                       sx, sy, pal_RGBA(22, 36, 32, 255));

        // 2. Great Plains & Mississippi Valley fertile basin
        fillUsaPolygon({ {355,110},{580,115},{615,320},{580,465},{470,515},{370,410},{340,240} },
                       sx, sy, pal_RGBA(28, 48, 38, 235));

        // 3. Appalachian Highlands (Eastern ridge)
        fillUsaPolygon({ {655,345},{755,215},{835,130},{855,165},{770,295},{695,385} },
                       sx, sy, pal_RGBA(44, 52, 42, 235));

        // 4. Cascade & Sierra Nevada Western Mountain Belt
        fillUsaPolygon({ {110,115},{175,115},{185,375},{155,395},{115,295} },
                       sx, sy, pal_RGBA(52, 54, 48, 240));

        // 5. Rocky Mountain Spine (High-altitude elevation zone)
        fillUsaPolygon({ {215,110},{345,110},{365,245},{355,415},{275,415},{225,270} },
                       sx, sy, pal_RGBA(64, 62, 56, 245));

        // 6. High Alpine Ridge Crest (Snow/granite peaks along Continental Divide)
        fillUsaPolygon({ {245,125},{315,125},{335,255},{325,385},{290,385},{265,245} },
                       sx, sy, pal_RGBA(92, 96, 102, 210));

        // 7. Subtle topographic contour lines & Mississippi River channel
        iV_Line(static_cast<int>(535 * sx), static_cast<int>(135 * sy), static_cast<int>(568 * sx), static_cast<int>(310 * sy), pal_RGBA(35, 95, 145, 180));
        iV_Line(static_cast<int>(568 * sx), static_cast<int>(310 * sy), static_cast<int>(575 * sx), static_cast<int>(475 * sy), pal_RGBA(35, 95, 145, 180));
    }

    void drawThickNeonSegment(int x0, int y0, int x1, int y1, PIELIGHT halo, PIELIGHT core, PIELIGHT filament)
    {
        // 1. Soft 5px outer neon bloom
        for (int ox = -2; ox <= 2; ++ox)
        {
            for (int oy = -2; oy <= 2; ++oy)
            {
                if (std::abs(ox) + std::abs(oy) <= 3)
                {
                    iV_Line(x0 + ox, y0 + oy, x1 + ox, y1 + oy, halo);
                }
            }
        }
        // 2. Bright 3px faction-tinted neon tube
        iV_Line(x0 - 1, y0, x1 - 1, y1, core);
        iV_Line(x0 + 1, y0, x1 + 1, y1, core);
        iV_Line(x0, y0 - 1, x1, y1 - 1, core);
        iV_Line(x0, y0 + 1, x1, y1 + 1, core);
        // 3. Crisp white-hot inner filament sealing any subpixel seam
        iV_Line(x0, y0, x1, y1, filament);
    }

    void drawUsaRadarSweep(int mapX, int mapY, int mapW, int mapH)
    {
        uint64_t ms = SDL_GetTicks();
        float cycle = static_cast<float>(ms % 4200ULL) / 4200.0f;
        int scanX = mapX + 4 + static_cast<int>(cycle * static_cast<float>(mapW - 8));

        for (int b = 1; b <= 16; ++b)
        {
            int bx0 = scanX - b * 6;
            int bx1 = bx0 + 6;
            if (bx1 <= mapX + 4) continue;
            bx0 = std::max(mapX + 4, bx0);
            uint8_t alpha = static_cast<uint8_t>(std::max(0, 75 - b * 5));
            pie_UniTransBoxFill(bx0, mapY + 4, bx1, mapY + mapH - 4, pal_RGBA(45, 225, 255, alpha));
        }

        iV_Line(scanX, mapY + 4, scanX, mapY + mapH - 4, pal_RGBA(175, 252, 255, 250));
        if (scanX - 1 > mapX + 4) iV_Line(scanX - 1, mapY + 4, scanX - 1, mapY + mapH - 4, pal_RGBA(70, 215, 255, 195));
    }
}
void renderUsaOverworldUi()
{
    UsaCampaignState& camp = getUsaCampaignState();
    const int sw = std::max(1024, static_cast<int>(screenWidth));
    const int sh = std::max(700, static_cast<int>(screenHeight));
    const float sx = static_cast<float>(sw) / 1360.0f;
    const float sy = static_cast<float>(sh) / 780.0f;
    const int mx = mouseX(), my = mouseY();

    pie_BoxFill(0, 0, sw, sh, pal_RGBA(8, 13, 21, 255));
    for (int gx = 0; gx < sw; gx += 48) iV_Line(gx, 0, gx, sh, pal_RGBA(16, 26, 40, 140));
    for (int gy = 0; gy < sh; gy += 48) iV_Line(0, gy, sw, gy, pal_RGBA(16, 26, 40, 140));

    drawWzPlate(static_cast<int>(15 * sx), static_cast<int>(12 * sy), static_cast<int>(1330 * sx), static_cast<int>(44 * sy), pal_RGBA(15, 24, 38, 250), pal_RGBA(70, 135, 195, 255), true);
    int ownedCount = 0, totalIncome = 0;
    for (const auto& s : camp.states) if (s.owner == UsaFaction::THE_PROJECT) { ownedCount++; totalIncome += s.powerIncome; }
    std::string header = "WARZONE 2020 // GLOBAL RECLAMATION   |   TURN: " + std::to_string(camp.turn) +
                         "   |   POWER: " + std::to_string(camp.power) + " (+" + std::to_string(totalIncome) + "/T)" +
                         "   |   SECTORS: " + std::to_string(ownedCount) + "/" + std::to_string(camp.states.size());
    drawWzPng("image_intel_campaign.png", static_cast<int>(22 * sx), static_cast<int>(18 * sy), 28, 28);
    iV_DrawText(header.c_str(), 58.0f * sx, 38.0f * sy, font_regular);

    int sysBtnX = static_cast<int>(1175 * sx), sysBtnY = static_cast<int>(17 * sy), sysBtnW = static_cast<int>(160 * sx), sysBtnH = static_cast<int>(34 * sy);
    drawWzButton(sysBtnX, sysBtnY, sysBtnW, sysBtnH, "SYSTEM MENU",
                 systemMenuOpen ? pal_RGBA(45, 105, 175, 255) : pal_RGBA(24, 42, 68, 255),
                 pal_RGBA(135, 215, 255, 255), hitRect(mx, my, sysBtnX, sysBtnY, sysBtnW, sysBtnH));

    int mapX = static_cast<int>(15 * sx), mapY = static_cast<int>(65 * sy), mapW = static_cast<int>(880 * sx), mapH = static_cast<int>(655 * sy);
    drawWzPlate(mapX, mapY, mapW, mapH, pal_RGBA(9, 17, 28, 255), pal_RGBA(55, 105, 155, 255), true);

    const float mapScaleY = sy * 1.24f, mapOffY = -20.0f * sy;
    drawUsaTerrainUnderMap(sx, mapScaleY);

    const UsaVec2 hoverPt{ static_cast<float>(mx) / sx, (static_cast<float>(my) - mapOffY) / mapScaleY };
    for (int i = 0; i < static_cast<int>(camp.states.size()); ++i)
    {
        const Territory& st = camp.states[i];
        bool isSel = (i == camp.selectedIndex), isHov = usaPointInPolygon(hoverPt, st.polygon);
        UsaColor c = getUsaFactionColor(st.owner, isSel || isHov);
        std::vector<UsaVec2> shifted;
        for (const auto& pt : st.polygon) shifted.push_back({ pt.x, pt.y + (mapOffY / mapScaleY) });
        fillUsaPolygon(shifted, sx, mapScaleY, pal_RGBA(c.r, c.g, c.b, isSel ? 185 : (isHov ? 160 : 128)));
    }

    for (int i = 0; i < static_cast<int>(camp.states.size()); ++i)
    {
        const Territory& st = camp.states[i];
        bool isSel = (i == camp.selectedIndex);
        UsaColor c = getUsaFactionColor(st.owner, true);
        PIELIGHT halo = isSel ? pal_RGBA(255, 220, 60, 95) : pal_RGBA(c.r, c.g, c.b, 55);
        PIELIGHT core = isSel ? pal_RGBA(255, 245, 130, 255) : pal_RGBA(std::min(255, c.r + 55), std::min(255, c.g + 55), std::min(255, c.b + 55), 220);
        PIELIGHT fil  = isSel ? pal_RGBA(255, 255, 235, 255) : pal_RGBA(std::min(255, c.r + 120), std::min(255, c.g + 120), std::min(255, c.b + 120), 255);
        for (size_t p = 0; p < st.polygon.size(); ++p)
        {
            const UsaVec2& a = st.polygon[p]; const UsaVec2& b = st.polygon[(p + 1) % st.polygon.size()];
            int x0 = static_cast<int>(a.x * sx), y0 = static_cast<int>(a.y * mapScaleY + mapOffY);
            int x1 = static_cast<int>(b.x * sx), y1 = static_cast<int>(b.y * mapScaleY + mapOffY);
            drawThickNeonSegment(x0, y0, x1, y1, halo, core, fil);
            pie_BoxFill(x0 - 1, y0 - 1, x0 + 2, y0 + 2, fil);
        }
        iV_DrawText(st.code.c_str(), st.labelPos.x * sx, (st.labelPos.y + 10.0f) * mapScaleY + mapOffY, font_regular);
    }

    drawUsaRadarSweep(mapX, mapY, mapW, mapH);
    iV_DrawText("FACTIONS:  [BLUE] THE PROJECT   [ORANGE] NEW PARADIGM   [RED] THE COLLECTIVE   [PURPLE] NEXUS   [TEAL] SCAV SYNDICATE", 28.0f * sx, 706.0f * sy, font_regular);
    const int tabStartX = static_cast<int>(910 * sx), tabY = static_cast<int>(65 * sy), tabW = static_cast<int>(105 * sx), tabH = static_cast<int>(36 * sy), tabGap = static_cast<int>(5 * sx);
    std::array<const char*, 4> tabNames = {{ "SECTOR", "RESEARCH", "DESIGN", "GENERAL" }};
    for (int tb = 0; tb < 4; ++tb)
    {
        int tx = tabStartX + tb * (tabW + tabGap);
        bool active = (activeRightTab == tb);
        drawWzButton(tx, tabY, tabW, tabH, tabNames[tb],
            active ? pal_RGBA(32, 86, 145, 255) : pal_RGBA(16, 24, 36, 250),
            active ? pal_RGBA(135, 225, 255, 255) : pal_RGBA(60, 100, 145, 255),
            !systemMenuOpen && hitRect(mx, my, tx, tabY, tabW, tabH));
    }

    drawWzPlate(static_cast<int>(910 * sx), static_cast<int>(108 * sy), static_cast<int>(435 * sx), static_cast<int>(612 * sy), pal_RGBA(16, 24, 36, 250), pal_RGBA(65, 115, 165, 255), true);
    const int bx = static_cast<int>(922 * sx), bw = static_cast<int>(411 * sx);

    if (activeRightTab == 0)
    {
        const Territory& sel = camp.states[camp.selectedIndex];
        drawWzPlate(bx, static_cast<int>(122 * sy), bw, static_cast<int>(126 * sy), pal_RGBA(12, 18, 28, 250), pal_RGBA(50, 85, 125, 255), false);
        iV_DrawText(("SECTOR: " + sel.name + " [" + sel.code + "]").c_str(), 934.0f * sx, 146.0f * sy, font_regular);
        iV_DrawText(("FACTION: " + getUsaFactionName(sel.owner)).c_str(), 934.0f * sx, 172.0f * sy, font_regular);
        iV_DrawText(("POWER YIELD: +" + std::to_string(sel.powerIncome) + " PWR / TURN").c_str(), 934.0f * sx, 198.0f * sy, font_regular);
        iV_DrawText(("COMBAT ZONE: " + sel.wzMap + "  |  ASSETS: " + std::to_string(sel.savedEntities.size())).c_str(), 934.0f * sx, 224.0f * sy, font_regular);

        bool owned = (sel.owner == UsaFaction::THE_PROJECT);
        int depY = static_cast<int>(265 * sy), depH = static_cast<int>(44 * sy);
        drawWzButton(bx, depY, bw, depH, owned ? "ENTER BASE COMMAND & GARRISON" : "LAUNCH TACTICAL INVASION",
            owned ? pal_RGBA(24, 78, 132, 255) : pal_RGBA(135, 32, 32, 255),
            owned ? pal_RGBA(95, 185, 255, 255) : pal_RGBA(255, 105, 105, 255), !systemMenuOpen && hitRect(mx, my, bx, depY, bw, depH));
    }
    else if (activeRightTab == 1)
    {
        iV_DrawText("STRATEGIC RESEARCH MODULES:", 926.0f * sx, 134.0f * sy, font_regular);
        for (int t = 0; t < static_cast<int>(camp.techs.size()); ++t)
        {
            const TechUpgrade& tech = camp.techs[t];
            int ty = static_cast<int>((150 + t * 92) * sy), th = static_cast<int>(78 * sy);
            drawWzButton(bx, ty, bw, th, (tech.unlocked ? "[ONLINE] " : "[" + std::to_string(tech.cost) + " PWR] ") + tech.label,
                tech.unlocked ? pal_RGBA(22, 78, 54, 255) : pal_RGBA(24, 34, 48, 255),
                tech.unlocked ? pal_RGBA(95, 225, 155, 255) : pal_RGBA(85, 140, 195, 255), !systemMenuOpen && hitRect(mx, my, bx, ty, bw, th));
        }
    }
    else if (activeRightTab == 2)
    {
        iV_DrawText("EXPEDITIONARY UNIT BLUEPRINT:", 926.0f * sx, 138.0f * sy, font_regular);
        const auto& curB = camp.bodies[selBodyIdx]; const auto& curP = camp.props[selPropIdx]; const auto& curW = camp.weapons[selWeapIdx];
        int unitCost = curB.cost + curP.cost + curW.cost, bh = static_cast<int>(32 * sy);
        drawWzButton(bx, static_cast<int>(158 * sy), bw, bh, "<  CHASSIS: " + curB.name + "  >", pal_RGBA(24, 34, 48, 255), pal_RGBA(85, 140, 195, 255), !systemMenuOpen && hitRect(mx, my, bx, static_cast<int>(158 * sy), bw, bh));
        drawWzButton(bx, static_cast<int>(198 * sy), bw, bh, "<  DRIVE:   " + curP.name + "  >", pal_RGBA(24, 34, 48, 255), pal_RGBA(85, 140, 195, 255), !systemMenuOpen && hitRect(mx, my, bx, static_cast<int>(198 * sy), bw, bh));
        drawWzButton(bx, static_cast<int>(238 * sy), bw, bh, "<  TURRET:  " + curW.name + "  >", pal_RGBA(24, 34, 48, 255), pal_RGBA(85, 140, 195, 255), !systemMenuOpen && hitRect(mx, my, bx, static_cast<int>(238 * sy), bw, bh));
        drawWzButton(bx, static_cast<int>(290 * sy), bw, static_cast<int>(38 * sy), "+ ASSEMBLE DIVISION UNIT (" + std::to_string(unitCost) + " PWR)",
                     pal_RGBA(26, 82, 136, 255), pal_RGBA(110, 195, 255, 255), !systemMenuOpen && hitRect(mx, my, bx, static_cast<int>(290 * sy), bw, static_cast<int>(38 * sy)));
        iV_DrawText("ACTIVE EXPEDITIONARY ROSTER:", 926.0f * sx, 362.0f * sy, font_regular);
        for (size_t i = 0; i < std::min<size_t>(9, camp.squad.size()); ++i)
            iV_DrawText(("[" + std::to_string(camp.squad[i].count) + "x] " + camp.squad[i].name).c_str(), 928.0f * sx, (390.0f + i * 26.0f) * sy, font_regular);
    }
    else if (activeRightTab == 3)
    {
        drawWzPlate(bx, static_cast<int>(120 * sy), bw, static_cast<int>(124 * sy), pal_RGBA(12, 18, 28, 250), pal_RGBA(245, 195, 75, 255), false);
        iV_DrawText("UNIT: DRAGON COMMAND GENERAL [BUILTIN HQ]", 934.0f * sx, 142.0f * sy, font_regular);
        iV_DrawText(("HULL: " + camp.general.bodyId + "  |  DRIVE: " + camp.general.propId).c_str(), 934.0f * sx, 166.0f * sy, font_regular);
        iV_DrawText(("SLOT 1: " + camp.general.weapon1Id + "  |  SLOT 2: " + camp.general.weapon2Id).c_str(), 934.0f * sx, 190.0f * sy, font_regular);
        iV_DrawText("COMMAND CORE: Built-In (0 Turret Slots Used)", 934.0f * sx, 214.0f * sy, font_regular);

        drawWzButton(bx, static_cast<int>(254 * sy), bw, static_cast<int>(38 * sy), "OPEN GENERAL DESIGN BAY (CUSTOMIZE)",
                     pal_RGBA(32, 76, 132, 255), pal_RGBA(125, 210, 255, 255), !systemMenuOpen && !generalDesignBayOpen && hitRect(mx, my, bx, static_cast<int>(254 * sy), bw, static_cast<int>(38 * sy)));

        for (int g = 0; g < static_cast<int>(camp.general.upgrades.size()); ++g)
        {
            const auto& node = camp.general.upgrades[g];
            int gy = static_cast<int>((304 + g * 58) * sy);
            int gh = static_cast<int>(48 * sy);
            int cost = node.baseCost + node.level * 120;
            bool maxed = (node.level >= node.maxLevel);
            std::string btnLabel = "[LVL " + std::to_string(node.level) + "/" + std::to_string(node.maxLevel) + "] " +
                                   node.label + (maxed ? " [MAX]" : " (" + std::to_string(cost) + "P)");
            drawWzButton(bx, gy, bw, gh, btnLabel,
                maxed ? pal_RGBA(24, 82, 56, 255) : pal_RGBA(26, 38, 56, 255),
                maxed ? pal_RGBA(95, 235, 160, 255) : pal_RGBA(245, 195, 75, 255),
                !systemMenuOpen && !generalDesignBayOpen && hitRect(mx, my, bx, gy, bw, gh));
        }
    }
    // Dedicated END TURN Button at bottom of Right Console
    int etY = static_cast<int>(666 * sy), etH = static_cast<int>(42 * sy);
    drawWzButton(bx, etY, bw, etH, "END TURN  (+" + std::to_string(totalIncome) + " PWR)",
                 pal_RGBA(24, 92, 60, 255), pal_RGBA(95, 235, 160, 255), !systemMenuOpen && hitRect(mx, my, bx, etY, bw, etH));

    // Bottom Telemetry Ticker
    drawWzPlate(static_cast<int>(15 * sx), static_cast<int>(730 * sy), static_cast<int>(1330 * sx), static_cast<int>(36 * sy), pal_RGBA(12, 18, 28, 255), pal_RGBA(55, 95, 140, 255), false);
    iV_DrawText(("SYS // " + overworldStatus).c_str(), 28.0f * sx, 753.0f * sy, font_regular);

    // Modal System Menu Overlay (Save, Load, Audio/GFX Options, New Campaign, Quit, Resume)
    if (generalDesignBayOpen)
    {
        pie_UniTransBoxFill(0, 0, sw, sh, pal_RGBA(3, 7, 16, 215));
        const int dw = static_cast<int>(920 * sx), dh = static_cast<int>(540 * sy);
        const int dx0 = (sw - dw) / 2, dy0 = static_cast<int>(105 * sy);

        drawWzPlate(dx0, dy0, dw, dh, pal_RGBA(10, 20, 42, 252), pal_RGBA(64, 145, 230, 255), true);
        drawWzPlate(dx0 + 6, dy0 + 6, dw - 12, static_cast<int>(34 * sy), pal_RGBA(16, 36, 74, 255), pal_RGBA(85, 170, 250, 255), false);
        drawWzPng("image_design_down.png", dx0 + 12, dy0 + 8, 28, 28);
        iV_DrawText("UNIT DESIGN // DRAGON COMMAND GENERAL (BUILT-IN COMMAND BRAIN)", static_cast<float>(dx0 + 46), static_cast<float>(dy0 + 28), font_regular);

        // Top-Right [X] Close Button
        int closeX = dx0 + dw - static_cast<int>(44 * sx), closeY = dy0 + 6, closeW = static_cast<int>(38 * sx), closeH = static_cast<int>(34 * sy);
        drawWzPlate(closeX, closeY, closeW, closeH, pal_RGBA(120, 32, 32, 255), pal_RGBA(255, 115, 105, 255), false);
        iV_DrawText("X", static_cast<float>(closeX + 14), static_cast<float>(closeY + 22), font_regular);

        // 4 Native Warzone 2100 Design Category Buttons (BODY, DRIVE, TURRET 1, TURRET 2)
        const int catX = dx0 + static_cast<int>(20 * sx), catY = dy0 + static_cast<int>(52 * sy);
        const int catW = static_cast<int>(102 * sx), catH = static_cast<int>(38 * sy), catGap = static_cast<int>(6 * sx);
        std::array<const char*, 4> catLabels = {{ "BODY", "DRIVE", "SLOT 1", "SLOT 2" }};
        std::array<const char*, 4> catPngUp   = {{ "image_des_body.png", "image_des_propulsion.png", "image_des_turret.png", "image_des_turret.png" }};
        std::array<const char*, 4> catPngDown = {{ "image_des_bodyh.png", "image_des_propulsionh.png", "image_des_turreth.png", "image_des_turreth.png" }};
        for (int c = 0; c < 4; ++c)
        {
            int tx = catX + c * (catW + catGap);
            bool active = (generalDesignCategory == c);
            bool hov = hitRect(mx, my, tx, catY, catW, catH);
            drawWzPlate(tx, catY, catW, catH,
                active ? pal_RGBA(28, 78, 148, 255) : (hov ? pal_RGBA(20, 44, 84, 255) : pal_RGBA(14, 26, 52, 255)),
                active ? pal_RGBA(145, 225, 255, 255) : pal_RGBA(55, 110, 175, 255), false);
            drawWzPng(active ? catPngDown[c] : catPngUp[c], tx + 6, catY + 4, 30, 30);
            iV_DrawText(catLabels[c], static_cast<float>(tx + 42), static_cast<float>(catY + (catH / 2) + 4), font_regular);
        }

        const std::vector<GeneralPartOption>* activeList = &camp.general.bodyOptions;
        std::string activeId = camp.general.bodyId;
        const char* cardPng = "image_des_body.png";
        if (generalDesignCategory == 1) { activeList = &camp.general.propOptions; activeId = camp.general.propId; cardPng = "image_des_propulsion.png"; }
        else if (generalDesignCategory == 2) { activeList = &camp.general.weap1Options; activeId = camp.general.weapon1Id; cardPng = "image_des_turret.png"; }
        else if (generalDesignCategory == 3) { activeList = &camp.general.weap2Options; activeId = camp.general.weapon2Id; cardPng = "image_des_turret.png"; }

        const int cardX = dx0 + static_cast<int>(20 * sx), cardW = static_cast<int>(426 * sx), cardH = static_cast<int>(54 * sy);
        for (int idx = 0; idx < static_cast<int>(activeList->size()); ++idx)
        {
            const auto& item = (*activeList)[idx];
            int cardY = dy0 + static_cast<int>((102 + idx * 62) * sy);
            bool selected = (item.id == activeId);
            bool hov = hitRect(mx, my, cardX, cardY, cardW, cardH);

            drawWzPlate(cardX, cardY, cardW, cardH,
                selected ? pal_RGBA(24, 66, 128, 255) : (hov ? pal_RGBA(20, 44, 84, 255) : pal_RGBA(13, 24, 48, 255)),
                selected ? pal_RGBA(255, 215, 80, 255) : pal_RGBA(58, 120, 195, 255), false);

            drawWzPng(cardPng, cardX + 10, cardY + 11, 32, 32);
            if (selected) drawWzPng("image_des_hilight.png", cardX + 4, cardY + 4, cardW - 8, cardH - 8);

            iV_DrawText(item.name.c_str(), static_cast<float>(cardX + static_cast<int>(58 * sx)), static_cast<float>(cardY + 22), font_regular);
            std::string subLine = "RATING: " + std::to_string(item.statBonus) + "%   |   ID: " + item.id;
            iV_DrawText(subLine.c_str(), static_cast<float>(cardX + static_cast<int>(58 * sx)), static_cast<float>(cardY + 42), font_regular);
        }
        const int rx = dx0 + static_cast<int>(466 * sx), rw = static_cast<int>(432 * sx);
        const int prevY = dy0 + static_cast<int>(52 * sy), prevH = static_cast<int>(215 * sy);
        drawWzPlate(rx, prevY, rw, prevH, pal_RGBA(6, 12, 26, 255), pal_RGBA(64, 145, 230, 255), true);
        for (int gx = rx + 24; gx < rx + rw; gx += 32) iV_Line(gx, prevY + 4, gx, prevY + prevH - 4, pal_RGBA(16, 36, 68, 130));
        for (int gy = prevY + 24; gy < prevY + prevH; gy += 32) iV_Line(rx + 4, gy, rx + rw - 4, gy, pal_RGBA(16, 36, 68, 130));

        float angle = static_cast<float>(SDL_GetTicks() % 6000ULL) * (6.28318f / 6000.0f);
        float ca = std::cos(angle), sa = std::sin(angle);
        int cx = rx + rw / 2, cy = prevY + prevH / 2 + 6;
        auto proj = [&](float lx, float ly, float lz) -> std::pair<int, int> {
            float rx3 = lx * ca - ly * sa;
            float ry3 = lx * sa + ly * ca;
            return { cx + static_cast<int>(rx3 * sx), cy + static_cast<int>((ry3 * 0.52f - lz) * sy) };
        };

        auto drawBox3D = [&](float x0, float y0, float z0, float x1, float y1, float z1, PIELIGHT col) {
            auto p000 = proj(x0,y0,z0), p100 = proj(x1,y0,z0), p110 = proj(x1,y1,z0), p010 = proj(x0,y1,z0);
            auto p001 = proj(x0,y0,z1), p101 = proj(x1,y0,z1), p111 = proj(x1,y1,z1), p011 = proj(x0,y1,z1);
            iV_Line(p000.first,p000.second,p100.first,p100.second,col); iV_Line(p100.first,p100.second,p110.first,p110.second,col);
            iV_Line(p110.first,p110.second,p010.first,p010.second,col); iV_Line(p010.first,p010.second,p000.first,p000.second,col);
            iV_Line(p001.first,p001.second,p101.first,p101.second,col); iV_Line(p101.first,p101.second,p111.first,p111.second,col);
            iV_Line(p111.first,p111.second,p011.first,p011.second,col); iV_Line(p011.first,p011.second,p001.first,p001.second,col);
            iV_Line(p000.first,p000.second,p001.first,p001.second,col); iV_Line(p100.first,p100.second,p101.first,p101.second,col);
            iV_Line(p110.first,p110.second,p111.first,p111.second,col); iV_Line(p010.first,p010.second,p011.first,p011.second,col);
        };
        drawBox3D(-68, -42, -10, 68, -26, 8, pal_RGBA(95, 165, 235, 220));
        drawBox3D(-68,  26, -10, 68,  42, 8, pal_RGBA(95, 165, 235, 220));
        drawBox3D(-56, -26,  -4, 56,  26, 18, pal_RGBA(115, 215, 255, 255));
        drawBox3D(-22, -22,  18, 12,  -4, 32, pal_RGBA(255, 125, 95, 255));
        drawBox3D(-22,   4,  18, 12,  22, 32, pal_RGBA(255, 125, 95, 255));
        drawBox3D( 12, -15,  23, 66, -11, 27, pal_RGBA(255, 175, 120, 255));
        drawBox3D( 12,  11,  23, 66,  15, 27, pal_RGBA(255, 175, 120, 255));
        drawBox3D(-38, -10,  18, -22, 10, 36, pal_RGBA(255, 225, 95, 255));
        iV_DrawText("DRAGON TWIN-TURRET TURNTABLE  [CMD BRAIN BUILT-IN]", static_cast<float>(rx + 12), static_cast<float>(prevY + 20), font_regular);

        auto findStat = [](const std::string& id, const std::vector<GeneralPartOption>& opts) -> int {
            for (const auto& o : opts) if (o.id == id) return o.statBonus;
            return 80;
        };
        int sBody = findStat(camp.general.bodyId, camp.general.bodyOptions);
        int sProp = findStat(camp.general.propId, camp.general.propOptions);
        int sW1   = findStat(camp.general.weapon1Id, camp.general.weap1Options);
        int sW2   = findStat(camp.general.weapon2Id, camp.general.weap2Options);

        const int statY = prevY + prevH + static_cast<int>(10 * sy), statH = static_cast<int>(190 * sy);
        drawWzPlate(rx, statY, rw, statH, pal_RGBA(12, 24, 48, 255), pal_RGBA(58, 125, 205, 255), false);

        auto drawWzSegmentedBar = [&](int rowY, const char* lbl, int pct, PIELIGHT litCol) {
            iV_DrawText(lbl, static_cast<float>(rx + 14), static_cast<float>(rowY + 13), font_regular);
            int segX0 = rx + static_cast<int>(138 * sx);
            int totalSegs = 20, segW = static_cast<int>(11 * sx), segGap = static_cast<int>(2 * sx);
            int litCount = std::max(1, std::min(totalSegs, (pct * totalSegs) / 100));
            for (int s = 0; s < totalSegs; ++s)
            {
                int sx0 = segX0 + s * (segW + segGap);
                pie_BoxFill(sx0, rowY, sx0 + segW, rowY + 14, (s < litCount) ? litCol : pal_RGBA(18, 32, 58, 255));
            }
        };
        drawWzPng("image_des_armour_kinetic.png", rx + 8, statY + static_cast<int>(12 * sy), 22, 22);
        drawWzSegmentedBar(statY + static_cast<int>(16 * sy),  "  ARMOR:",  sBody, pal_RGBA(70, 185, 255, 255));
        drawWzPng("image_des_crosscountry.png", rx + 8, statY + static_cast<int>(44 * sy), 22, 22);
        drawWzSegmentedBar(statY + static_cast<int>(48 * sy),  "  DRIVE:",  sProp, pal_RGBA(80, 235, 145, 255));
        drawWzPng("image_des_damage.png", rx + 8, statY + static_cast<int>(76 * sy), 22, 22);
        drawWzSegmentedBar(statY + static_cast<int>(80 * sy),  "  SLOT 1:", sW1,   pal_RGBA(255, 115, 85, 255));
        drawWzPng("image_des_firerate.png", rx + 8, statY + static_cast<int>(108 * sy), 22, 22);
        drawWzSegmentedBar(statY + static_cast<int>(112 * sy), "  SLOT 2:", sW2,   pal_RGBA(255, 155, 75, 255));
        drawWzPng("image_des_command.png", rx + 8, statY + static_cast<int>(140 * sy), 22, 22);
        drawWzSegmentedBar(statY + static_cast<int>(144 * sy), "  HQ CORE:", 100,  pal_RGBA(255, 220, 85, 255));

        const int btnW = (rw - static_cast<int>(12 * sx)) / 2;
        const int btnY = dy0 + dh - static_cast<int>(60 * sy), btnH = static_cast<int>(44 * sy);
        (void)hitRect(mx, my, rx, btnY, btnW, btnH);
        bool hovExit = hitRect(mx, my, rx + btnW + static_cast<int>(12 * sx), btnY, btnW, btnH);

        drawWzPlate(rx, btnY, btnW, btnH, pal_RGBA(22, 82, 56, 255), pal_RGBA(95, 235, 160, 255), false);
        drawWzPng("image_des_save.png", rx + 10, btnY + 8, 28, 28);
        iV_DrawText("SAVE DESIGN", static_cast<float>(rx + 46), static_cast<float>(btnY + 26), font_regular);

        int exX = rx + btnW + static_cast<int>(12 * sx);
        drawWzPlate(exX, btnY, btnW, btnH, pal_RGBA(115, 36, 32, 255), pal_RGBA(245, 115, 105, 255), false);
        drawWzPng(hovExit ? "image_des_deleteh.png" : "image_des_delete.png", exX + 10, btnY + 8, 28, 28);
        iV_DrawText("CLOSE BAY", static_cast<float>(exX + 46), static_cast<float>(btnY + 26), font_regular);
    }

    if (systemMenuOpen)
    {
        pie_UniTransBoxFill(0, 0, sw, sh, pal_RGBA(4, 8, 14, 185));
        const int mw = static_cast<int>(380 * sx), mh = static_cast<int>(375 * sy);
        const int mx0 = (sw - mw) / 2, my0 = static_cast<int>(165 * sy);
        const int mbw = mw - static_cast<int>(40 * sx), mbx = mx0 + static_cast<int>(20 * sx), mbh = static_cast<int>(40 * sy);

        drawWzPlate(mx0, my0, mw, mh, pal_RGBA(14, 22, 34, 255), pal_RGBA(110, 195, 255, 255), true);
        iV_DrawText("WARZONE 2020 // SYSTEM COMMAND MENU", static_cast<float>(mbx), static_cast<float>(my0 + 30), font_regular);

        drawWzButton(mbx, static_cast<int>(220 * sy), mbw, mbh, "SAVE CAMPAIGN", pal_RGBA(26, 68, 122, 255), pal_RGBA(115, 185, 255, 255), hitRect(mx, my, mbx, static_cast<int>(220 * sy), mbw, mbh));
        drawWzButton(mbx, static_cast<int>(270 * sy), mbw, mbh, "LOAD CAMPAIGN", pal_RGBA(26, 68, 122, 255), pal_RGBA(115, 185, 255, 255), hitRect(mx, my, mbx, static_cast<int>(270 * sy), mbw, mbh));
        drawWzButton(mbx, static_cast<int>(320 * sy), mbw, mbh, "AUDIO & GFX OPTIONS", pal_RGBA(32, 54, 82, 255), pal_RGBA(135, 215, 255, 255), hitRect(mx, my, mbx, static_cast<int>(320 * sy), mbw, mbh));
        drawWzButton(mbx, static_cast<int>(370 * sy), mbw, mbh, "NEW CAMPAIGN", pal_RGBA(115, 42, 36, 255), pal_RGBA(245, 120, 110, 255), hitRect(mx, my, mbx, static_cast<int>(370 * sy), mbw, mbh));
        drawWzButton(mbx, static_cast<int>(420 * sy), mbw, mbh, "QUIT TO MAIN MENU", pal_RGBA(115, 36, 32, 255), pal_RGBA(245, 115, 105, 255), hitRect(mx, my, mbx, static_cast<int>(420 * sy), mbw, mbh));
        drawWzButton(mbx, static_cast<int>(480 * sy), mbw, mbh, "RESUME COMMAND", pal_RGBA(24, 88, 58, 255), pal_RGBA(95, 225, 155, 255), hitRect(mx, my, mbx, static_cast<int>(480 * sy), mbw, mbh));
    }
}
