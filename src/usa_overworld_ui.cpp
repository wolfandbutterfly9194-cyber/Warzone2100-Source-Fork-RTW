#include "usa_overworld_ui.h"
#include "usa_campaign_state.h"
#include "usa_battle_setup.h"
#include "usa_metagame_bridge.h"

#include "lib/framework/frame.h"
#include "lib/framework/input.h"
#include "lib/framework/physfs_ext.h"
#include "lib/ivis_opengl/pieblitfunc.h"
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

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <string>

namespace
{
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
    if (keyPressed(KEY_ESC))
    {
        bUsaPrebattle = false;
        startTitleMenu();
        return false;
    }
    UsaCampaignState& camp = getUsaCampaignState();
    const int sw = std::max(1024, static_cast<int>(screenWidth));
    const int sh = std::max(700, static_cast<int>(screenHeight));
    const float sx = static_cast<float>(sw) / 1360.0f;
    const float sy = static_cast<float>(sh) / 780.0f;

    if (mousePressed(MOUSE_LMB))
    {
        const int mx = mouseX();
        const int my = mouseY();
        const UsaVec2 mapPt{ static_cast<float>(mx) / sx, static_cast<float>(my) / sy };

        // 1. Check 48-state USA map polygons
        for (int i = 0; i < static_cast<int>(camp.states.size()); ++i)
        {
            if (usaPointInPolygon(mapPt, camp.states[i].polygon))
            {
                camp.selectedIndex = i;
                overworldStatus = "Selected " + camp.states[i].name + " (" + camp.states[i].code + ") - Owner: " + getUsaFactionName(camp.states[i].owner);
                return true;
            }
        }

        const int sideX = static_cast<int>(910 * sx);
        const int btnW = static_cast<int>(405 * sx);
        const int btnX = static_cast<int>(925 * sx);

        // 2. Deploy / Base Build Button
        if (hitRect(mx, my, btnX, static_cast<int>(196 * sy), btnW, static_cast<int>(38 * sy)))
        {
            const Territory& sel = camp.states[camp.selectedIndex];
            if (sel.owner == UsaFaction::THE_PROJECT)
            {
                launchStateMission(camp.selectedIndex, true);
                return false;
            }
            bool adjacentOwned = false;
            for (const std::string& nb : sel.neighbors)
            {
                for (const Territory& st : camp.states)
                {
                    if (st.code == nb && st.owner == UsaFaction::THE_PROJECT) { adjacentOwned = true; break; }
                }
            }
            if (adjacentOwned)
            {
                launchStateMission(camp.selectedIndex, false);
                return false;
            }
            overworldStatus = "[!] Cannot invade " + sel.name + ": must border an owned state!";
            return true;
        }

        // 3. Tech Research Cards (Bottom Panel)
        for (int t = 0; t < static_cast<int>(camp.techs.size()); ++t)
        {
            int tx = static_cast<int>((30 + t * 215) * sx);
            int ty = static_cast<int>(632 * sy);
            int tw = static_cast<int>(205 * sx);
            int th = static_cast<int>(75 * sy);
            if (hitRect(mx, my, tx, ty, tw, th))
            {
                TechUpgrade& tech = camp.techs[t];
                if (!tech.unlocked && camp.power >= tech.cost)
                {
                    camp.power -= tech.cost;
                    tech.unlocked = true;
                    camp.saveToPhysfs();
                    overworldStatus = "Researched: " + tech.label;
                }
                return true;
            }
        }

        // 4. Unit Designer Component Selectors & Build Button
        int pBtnW = static_cast<int>(36 * sx);
        int rowH = static_cast<int>(26 * sy);
        if (hitRect(mx, my, btnX, static_cast<int>(388 * sy), pBtnW, rowH)) selBodyIdx = (selBodyIdx + static_cast<int>(camp.bodies.size()) - 1) % static_cast<int>(camp.bodies.size());
        if (hitRect(mx, my, btnX + btnW - pBtnW, static_cast<int>(388 * sy), pBtnW, rowH)) selBodyIdx = (selBodyIdx + 1) % static_cast<int>(camp.bodies.size());
        if (hitRect(mx, my, btnX, static_cast<int>(420 * sy), pBtnW, rowH)) selPropIdx = (selPropIdx + static_cast<int>(camp.props.size()) - 1) % static_cast<int>(camp.props.size());
        if (hitRect(mx, my, btnX + btnW - pBtnW, static_cast<int>(420 * sy), pBtnW, rowH)) selPropIdx = (selPropIdx + 1) % static_cast<int>(camp.props.size());
        if (hitRect(mx, my, btnX, static_cast<int>(452 * sy), pBtnW, rowH)) selWeapIdx = (selWeapIdx + static_cast<int>(camp.weapons.size()) - 1) % static_cast<int>(camp.weapons.size());
        if (hitRect(mx, my, btnX + btnW - pBtnW, static_cast<int>(452 * sy), pBtnW, rowH)) selWeapIdx = (selWeapIdx + 1) % static_cast<int>(camp.weapons.size());

        if (hitRect(mx, my, btnX, static_cast<int>(514 * sy), btnW, static_cast<int>(32 * sy)))
        {
            const auto& b = camp.bodies[selBodyIdx];
            const auto& p = camp.props[selPropIdx];
            const auto& w = camp.weapons[selWeapIdx];
            int cost = b.cost + p.cost + w.cost;
            if (camp.power >= cost)
            {
                camp.power -= cost;
                std::string bpName = b.name + " " + w.name;
                bool merged = false;
                for (auto& existing : camp.squad)
                {
                    if (existing.bodyId == b.id && existing.propId == p.id && existing.weaponId == w.id)
                    {
                        existing.count++;
                        merged = true;
                        break;
                    }
                }
                if (!merged) camp.squad.push_back({ bpName, b.id, p.id, w.id, cost, 1 });
                camp.saveToPhysfs();
                overworldStatus = "Added " + bpName + " to Expeditionary Squad (-" + std::to_string(cost) + " Power).";
            }
            return true;
        }

        // 5. End Turn / Save / New Campaign Buttons
        int actW = static_cast<int>(130 * sx);
        int actY = static_cast<int>(672 * sy);
        int actH = static_cast<int>(36 * sy);
        if (hitRect(mx, my, btnX, actY, actW, actH))
        {
            int income = 0;
            for (const auto& s : camp.states) if (s.owner == UsaFaction::THE_PROJECT) income += s.powerIncome;
            camp.turn++;
            camp.power += income;
            camp.saveToPhysfs();
            overworldStatus = "Turn " + std::to_string(camp.turn) + ": Collected +" + std::to_string(income) + " Power from owned states.";
            return true;
        }
        if (hitRect(mx, my, btnX + actW + 8, actY, actW, actH))
        {
            camp.saveToPhysfs();
            overworldStatus = "Campaign saved to usa_campaign/usa_campaign_save.txt.";
            return true;
        }
        if (hitRect(mx, my, btnX + (actW + 8) * 2, actY, actW, actH))
        {
            camp.initDefault();
            camp.saveToPhysfs();
            overworldStatus = "Started fresh USA Reclamation Campaign.";
            return true;
        }
        (void)sideX;
    }
    return true;
}
void renderUsaOverworldUi()
{
    UsaCampaignState& camp = getUsaCampaignState();
    const int sw = std::max(1024, static_cast<int>(screenWidth));
    const int sh = std::max(700, static_cast<int>(screenHeight));
    const float sx = static_cast<float>(sw) / 1360.0f;
    const float sy = static_cast<float>(sh) / 780.0f;

    pie_BoxFill(0, 0, sw, sh, pal_RGBA(10, 16, 26, 255));

    // Top Header Bar
    drawPanelBox(static_cast<int>(15 * sx), static_cast<int>(12 * sy), static_cast<int>(1330 * sx), static_cast<int>(44 * sy), pal_RGBA(18, 28, 44, 245), pal_RGBA(65, 105, 155, 255));
    int ownedCount = 0;
    for (const auto& s : camp.states) if (s.owner == UsaFaction::THE_PROJECT) ownedCount++;
    std::string header = "WARZONE 2100: USA RECLAMATION INITIATIVE   |   TURN: " + std::to_string(camp.turn) +
                         "   |   POWER: " + std::to_string(camp.power) +
                         "   |   STATES: " + std::to_string(ownedCount) + "/" + std::to_string(camp.states.size());
    iV_DrawText(header.c_str(), 28.0f * sx, 38.0f * sy, font_regular);

    // Map Frame & 48 Continental US State Polygons
    drawPanelBox(static_cast<int>(15 * sx), static_cast<int>(65 * sy), static_cast<int>(880 * sx), static_cast<int>(520 * sy), pal_RGBA(14, 24, 38, 245), pal_RGBA(55, 85, 115, 255));
    for (int i = 0; i < static_cast<int>(camp.states.size()); ++i)
    {
        const Territory& st = camp.states[i];
        bool isSel = (i == camp.selectedIndex);
        UsaColor c = getUsaFactionColor(st.owner, isSel);
        PIELIGHT fill = pal_RGBA(c.r, c.g, c.b, isSel ? 235 : 175);
        PIELIGHT edge = isSel ? pal_RGBA(255, 245, 140, 255) : pal_RGBA(15, 22, 32, 255);

        float minX = 9999.0f, minY = 9999.0f, maxX = 0.0f, maxY = 0.0f;
        for (size_t p = 0; p < st.polygon.size(); ++p)
        {
            const UsaVec2& a = st.polygon[p];
            const UsaVec2& b = st.polygon[(p + 1) % st.polygon.size()];
            minX = std::min(minX, a.x); minY = std::min(minY, a.y);
            maxX = std::max(maxX, a.x); maxY = std::max(maxY, a.y);
            iV_Line(static_cast<int>(a.x * sx), static_cast<int>(a.y * sy), static_cast<int>(b.x * sx), static_cast<int>(b.y * sy), edge);
        }
        pie_UniTransBoxFill(static_cast<int>((minX + 3) * sx), static_cast<int>((minY + 3) * sy), static_cast<int>((maxX - 3) * sx), static_cast<int>((maxY - 3) * sy), fill);
        iV_DrawText(st.code.c_str(), st.labelPos.x * sx, (st.labelPos.y + 10.0f) * sy, font_regular);
    }

    // Right Command & Unit Designer Panel
    const Territory& sel = camp.states[camp.selectedIndex];
    drawPanelBox(static_cast<int>(910 * sx), static_cast<int>(65 * sy), static_cast<int>(435 * sx), static_cast<int>(655 * sy), pal_RGBA(20, 28, 40, 245), pal_RGBA(60, 85, 115, 255));
    iV_DrawText(("SECTOR: " + sel.name + " (" + sel.code + ")").c_str(), 925.0f * sx, 92.0f * sy, font_regular);
    iV_DrawText(("OWNER: " + getUsaFactionName(sel.owner)).c_str(), 925.0f * sx, 116.0f * sy, font_regular);
    iV_DrawText(("INCOME: +" + std::to_string(sel.powerIncome) + " PWR   |   MAP: " + sel.wzMap).c_str(), 925.0f * sx, 140.0f * sy, font_regular);
    iV_DrawText(("BASE ENTITIES: " + std::to_string(sel.savedEntities.size()) + " Saved").c_str(), 925.0f * sx, 164.0f * sy, font_regular);

    bool owned = (sel.owner == UsaFaction::THE_PROJECT);
    drawActionBtn(static_cast<int>(925 * sx), static_cast<int>(196 * sy), static_cast<int>(405 * sx), static_cast<int>(38 * sy),
        owned ? "[ ENTER BASE BUILD & GARRISON ]" : "[ LAUNCH TACTICAL INVASION ]",
        owned ? pal_RGBA(35, 115, 185, 255) : pal_RGBA(185, 45, 45, 255), pal_RGBA(220, 235, 255, 255));

    // Unit Designer Section
    iV_DrawText("--- EXPEDITIONARY SQUAD DESIGNER ---", 925.0f * sx, 372.0f * sy, font_regular);
    const auto& curB = camp.bodies[selBodyIdx];
    const auto& curP = camp.props[selPropIdx];
    const auto& curW = camp.weapons[selWeapIdx];
    int unitCost = curB.cost + curP.cost + curW.cost;
    drawActionBtn(static_cast<int>(925 * sx), static_cast<int>(388 * sy), static_cast<int>(405 * sx), static_cast<int>(26 * sy), "<  BODY: " + curB.name + "  >", pal_RGBA(34, 46, 66, 255), pal_RGBA(90, 125, 165, 255));
    drawActionBtn(static_cast<int>(925 * sx), static_cast<int>(420 * sy), static_cast<int>(405 * sx), static_cast<int>(26 * sy), "<  PROP: " + curP.name + "  >", pal_RGBA(34, 46, 66, 255), pal_RGBA(90, 125, 165, 255));
    drawActionBtn(static_cast<int>(925 * sx), static_cast<int>(452 * sy), static_cast<int>(405 * sx), static_cast<int>(26 * sy), "<  WEAP: " + curW.name + "  >", pal_RGBA(34, 46, 66, 255), pal_RGBA(90, 125, 165, 255));
    drawActionBtn(static_cast<int>(925 * sx), static_cast<int>(514 * sy), static_cast<int>(405 * sx), static_cast<int>(32 * sy), "[ + RECRUIT UNIT (" + std::to_string(unitCost) + " PWR) ]", pal_RGBA(45, 120, 185, 255), pal_RGBA(125, 195, 250, 255));

    for (size_t i = 0; i < std::min<size_t>(3, camp.squad.size()); ++i)
    {
        std::string sqLine = std::to_string(camp.squad[i].count) + "x " + camp.squad[i].name;
        iV_DrawText(sqLine.c_str(), 925.0f * sx, (574.0f + i * 24.0f) * sy, font_regular);
    }

    // Turn / Save / Reset Buttons
    int actW = static_cast<int>(130 * sx);
    int actY = static_cast<int>(672 * sy);
    int actH = static_cast<int>(36 * sy);
    drawActionBtn(static_cast<int>(925 * sx), actY, actW, actH, "END TURN", pal_RGBA(35, 110, 75, 255), pal_RGBA(110, 215, 155, 255));
    drawActionBtn(static_cast<int>(925 * sx) + actW + 8, actY, actW, actH, "SAVE", pal_RGBA(45, 85, 145, 255), pal_RGBA(125, 175, 240, 255));
    drawActionBtn(static_cast<int>(925 * sx) + (actW + 8) * 2, actY, actW, actH, "NEW GAME", pal_RGBA(145, 55, 45, 255), pal_RGBA(235, 130, 120, 255));

    // Bottom Tech Research Panel & Status Bar
    drawPanelBox(static_cast<int>(15 * sx), static_cast<int>(595 * sy), static_cast<int>(880 * sx), static_cast<int>(125 * sy), pal_RGBA(20, 28, 40, 245), pal_RGBA(60, 85, 115, 255));
    iV_DrawText("STRATEGIC RESEARCH LABS (CLICK TO UNLOCK):", 28.0f * sx, 618.0f * sy, font_regular);
    for (int t = 0; t < static_cast<int>(camp.techs.size()); ++t)
    {
        const TechUpgrade& tech = camp.techs[t];
        int tx = static_cast<int>((30 + t * 215) * sx);
        int ty = static_cast<int>(632 * sy);
        drawActionBtn(tx, ty, static_cast<int>(205 * sx), static_cast<int>(75 * sy),
            (tech.unlocked ? "[UNLOCKED] " : "[" + std::to_string(tech.cost) + "P] ") + tech.id,
            tech.unlocked ? pal_RGBA(30, 95, 65, 255) : pal_RGBA(32, 44, 62, 255), pal_RGBA(85, 115, 150, 255));
    }

    drawPanelBox(static_cast<int>(15 * sx), static_cast<int>(730 * sy), static_cast<int>(1330 * sx), static_cast<int>(36 * sy), pal_RGBA(16, 22, 32, 255), pal_RGBA(50, 70, 95, 255));
    iV_DrawText(overworldStatus.c_str(), 28.0f * sx, 753.0f * sy, font_regular);
}
