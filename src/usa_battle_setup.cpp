#include "usa_battle_setup.h"

#include "lib/framework/frame.h"
#include "droid.h"
#include "map.h"
#include "power.h"
#include "research.h"
#include "stats.h"
#include "structure.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace
{
    STRUCTURE_STATS* findStructStats(const std::string& id)
    {
        for (UDWORD i = 0; i < numStructureStats; ++i)
        {
            if (asStructureStats[i].id.toUtf8() == id)
            {
                return &asStructureStats[i];
            }
        }
        return nullptr;
    }

    STRUCTURE* spawnNativeStruct(const std::string& id, int tileX, int tileY, UDWORD player)
    {
        STRUCTURE_STATS* psStats = findStructStats(id);
        if (psStats == nullptr) return nullptr;
        int wx = std::max(world_coord(4), std::min(world_coord(mapWidth - 5), world_coord(tileX)));
        int wy = std::max(world_coord(4), std::min(world_coord(mapHeight - 5), world_coord(tileY)));
        return buildStructure(psStats, static_cast<UDWORD>(wx), static_cast<UDWORD>(wy), player, true);
    }

    DROID* spawnNativeDroid(const std::string& name, const std::string& body, const std::string& prop, const std::string& weap, int tileX, int tileY, UDWORD player)
    {
        int bIdx = getCompFromID(COMP_BODY, WzString::fromUtf8(body));
        int pIdx = getCompFromID(COMP_PROPULSION, WzString::fromUtf8(prop));
        int wIdx = getCompFromID(COMP_WEAPON, WzString::fromUtf8(weap));
        if (bIdx < 0) bIdx = getCompFromID(COMP_BODY, "Body1REC");
        if (pIdx < 0) pIdx = getCompFromID(COMP_PROPULSION, "wheeled01");
        if (bIdx < 0 || pIdx < 0) return nullptr;

        DROID_TEMPLATE tmpl = {};
        tmpl.name = WzString::fromUtf8(name);
        tmpl.id = WzString::fromUtf8(name);
        tmpl.asParts[COMP_BODY] = bIdx;
        tmpl.asParts[COMP_PROPULSION] = pIdx;
        if (wIdx >= 0)
        {
            tmpl.numWeaps = 1;
            tmpl.asWeaps[0] = wIdx;
        }
        tmpl.droidType = droidTemplateType(&tmpl);

        int wx = std::max(world_coord(4), std::min(world_coord(mapWidth - 5), world_coord(tileX)));
        int wy = std::max(world_coord(4), std::min(world_coord(mapHeight - 5), world_coord(tileY)));
        DROID* psNew = reallyBuildDroid(&tmpl, Position(wx, wy, 0), player, false);
        if (psNew != nullptr)
        {
            droidSetBits(&tmpl, psNew);
        }
        return psNew;
    }

    void grantNativeResearch(const std::string& resId, UBYTE player)
    {
        for (UDWORD i = 0; i < asResearch.size(); ++i)
        {
            if (asResearch[i].id.toUtf8() == resId)
            {
                researchResult(i, player, false, nullptr, false);
                return;
            }
        }
    }
}
void usaApplyNativeBattleSetup()
{
    UsaCampaignState& camp = getUsaCampaignState();
    if (camp.activeTargetIndex < 0 || camp.activeTargetIndex >= static_cast<int>(camp.states.size()))
    {
        return;
    }

    Territory& target = camp.states[camp.activeTargetIndex];
    setPower(0, camp.power);

    int lzX = 24;
    int lzY = 24;
    for (STRUCTURE* psS : apsStructLists.at(0))
    {
        if (psS && !psS->died)
        {
            lzX = map_coord(psS->pos.x);
            lzY = map_coord(psS->pos.y);
            break;
        }
    }

    if (!camp.activeSessionIsBaseMode)
    {
        bool isScav = (target.owner == UsaFaction::SCAV_SYNDICATE);
        setPower(1, isScav ? 15000 : 6000);

        struct LayoutItem { const char* id; int dx; int dy; };
        if (isScav)
        {
            static const LayoutItem scavLayout[] = {
                { "A0BaBaFactory", 0, 0 }, { "A0BaBaFactory", 5, 0 },
                { "A0BaBaBunker", -4, -3 }, { "A0BaBaBunker", 2, -3 }, { "A0BaBaBunker", 8, -3 },
                { "A0BaBaGunTower", -4, 0 }, { "A0BaBaGunTower", 5, -3 },
                { "A0BaBaRocketPit", -4, -6 }, { "A0BaBaRocketPit", 5, -6 },
                { "A0BaBaFlameTower", -4, 3 }, { "A0BaBaFlameTower", 10, 3 }
            };

            int placed = 0;
            for (int gx = 18; gx <= mapWidth - 20 && placed < 5; gx += 22)
            {
                for (int gy = 18; gy <= mapHeight - 20 && placed < 5; gy += 22)
                {
                    if (std::hypot(gx - lzX, gy - lzY) < 38.0) continue;
                    if (terrainType(mapTile(gx, gy)) == TER_WATER || terrainType(mapTile(gx, gy)) == TER_CLIFFFACE) continue;
                    for (const auto& item : scavLayout)
                    {
                        spawnNativeStruct(item.id, gx + item.dx, gy + item.dy, 1);
                    }
                    spawnNativeDroid("Scav Raider", "B1BaBaPerson01", "BaBaLegs", "BaBaMG", gx + 1, gy + 3, 1);
                    spawnNativeDroid("Scav Trike", "B4body-sml-trike01", "BaBaProp", "Rocket-Pod", gx + 3, gy + 3, 1);
                    spawnNativeDroid("Scav Buggy", "B3body-sml-buggy01", "BaBaProp", "Flame1Mk1", gx + 5, gy + 3, 1);
                    placed++;
                }
            }
        }
        else
        {
            int ex = std::max(16, std::min(mapWidth - 16, mapWidth - lzX));
            int ey = std::max(16, std::min(mapHeight - 16, mapHeight - lzY));
            spawnNativeStruct("A0PowerGenerator", ex - 4, ey - 2, 1);
            spawnNativeStruct("A0PowerGenerator", ex - 4, ey + 2, 1);
            spawnNativeStruct("A0LightFactory", ex + 3, ey - 2, 1);
            spawnNativeStruct("A0LightFactory", ex + 3, ey + 2, 1);
            spawnNativeStruct("A0ResearchFacility", ex, ey - 4, 1);
        }
    }
    for (const auto& t : camp.techs)
    {
        if (t.unlocked) grantNativeResearch(t.wzResearchId, 0);
    }

    if (!target.savedEntities.empty())
    {
        for (const auto& ent : target.savedEntities)
        {
            if (ent.kind == "STRUCT" && ent.id != "A0CommandCentre" && ent.id != "A0ResourceExtractor" && ent.id != "A0ComDroidControl")
            {
                spawnNativeStruct(ent.id, ent.x, ent.y, 0);
            }
            else if (ent.kind == "DROID")
            {
                spawnNativeDroid(ent.name, ent.body, ent.prop, ent.weapon, ent.x, ent.y, 0);
            }
        }
    }
    else if (camp.activeSessionIsBaseMode)
    {
        spawnNativeStruct("A0PowerGenerator", lzX - 5, lzY - 2, 0);
        spawnNativeStruct("A0LightFactory", lzX + 3, lzY - 2, 0);
        spawnNativeDroid("Combat Engineer", "Body1REC", "wheeled01", "Spade1Mk1", lzX, lzY + 2, 0);
        spawnNativeDroid("Combat Engineer", "Body1REC", "wheeled01", "Spade1Mk1", lzX + 2, lzY + 2, 0);
    }

    if (!camp.activeSessionIsBaseMode)
    {
        int offsetIdx = 0;
        for (const auto& bp : camp.squad)
        {
            for (int c = 0; c < bp.count; ++c)
            {
                int dx = ((offsetIdx % 5) - 2) * 2;
                int dy = ((offsetIdx / 5) + 2) * 2;
                spawnNativeDroid(bp.name, bp.bodyId, bp.propId, bp.weaponId, lzX + dx, lzY + dy, 0);
                offsetIdx++;
            }
        }
    }
}

void usaExportSessionResults(bool playerWon)
{
    UsaCampaignState& camp = getUsaCampaignState();
    if (camp.activeTargetIndex < 0 || camp.activeTargetIndex >= static_cast<int>(camp.states.size())) return;

    Territory& target = camp.states[camp.activeTargetIndex];
    camp.power = getPower(0);

    if (camp.activeSessionIsBaseMode || playerWon)
    {
        if (!camp.activeSessionIsBaseMode && playerWon) target.owner = UsaFaction::THE_PROJECT;
        target.savedEntities.clear();
        for (STRUCTURE* psS : apsStructLists.at(0))
        {
            if (!psS || psS->died || psS->status != SS_BUILT) continue;
            SavedEntity e;
            e.kind = "STRUCT";
            e.id = psS->pStructureType->id.toUtf8();
            e.name = psS->pStructureType->name.toUtf8();
            e.x = map_coord(psS->pos.x);
            e.y = map_coord(psS->pos.y);
            target.savedEntities.push_back(e);
        }
        for (DROID* psD : apsDroidLists.at(0))
        {
            if (!psD || psD->died) continue;
            if (psD->droidType != DROID_CONSTRUCT && psD->droidType != DROID_CYBORG_CONSTRUCT) continue;
            SavedEntity e;
            e.kind = "DROID";
            e.name = "Combat Engineer";
            e.body = "Body1REC";
            e.prop = "wheeled01";
            e.weapon = "Spade1Mk1";
            e.x = map_coord(psD->pos.x);
            e.y = map_coord(psD->pos.y);
            target.savedEntities.push_back(e);
        }
    }
    camp.activeTargetIndex = -1;
    camp.saveToPhysfs();
}
