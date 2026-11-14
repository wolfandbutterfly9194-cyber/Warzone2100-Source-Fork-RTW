#include "usa_battle_setup.h"

#include "lib/framework/frame.h"
#include "droid.h"
#include "component.h"
#include "map.h"
#include "power.h"
#include "research.h"
#include "stats.h"
#include "structure.h"
#include "template.h"

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
        for (int r = 0; r <= 10; ++r)
        {
            for (int dx = -r; dx <= r; ++dx)
            {
                for (int dy = -r; dy <= r; ++dy)
                {
                    if (std::abs(dx) != r && std::abs(dy) != r) continue;
                    int tx = std::max(5, std::min(mapWidth - 6, tileX + dx));
                    int ty = std::max(5, std::min(mapHeight - 6, tileY + dy));
                    int wx = world_coord(tx) + TILE_UNITS / 2;
                    int wy = world_coord(ty) + TILE_UNITS / 2;
                    if (validLocation(psStats, Vector2i(wx, wy), 0, player, false))
                    {
                        STRUCTURE* built = buildStructure(psStats, static_cast<UDWORD>(wx), static_cast<UDWORD>(wy), player, true);
                        if (built != nullptr)
                        {
                            built->status = SS_BUILT;
                            built->body = built->structureBody();
                            return built;
                        }
                    }
                }
            }
        }
        return nullptr;
    }

    DROID_TEMPLATE* getOrRegisterTemplate(UDWORD player, const std::string& name, const std::string& body, const std::string& prop, const std::string& weap)
    {
        DROID_TEMPLATE* found = nullptr;
        enumerateTemplates(static_cast<int>(player), [&](DROID_TEMPLATE* psT) {
            if (psT && psT->name.toUtf8() == name) { found = psT; return false; }
            return true;
        });
        if (found != nullptr) return found;

        int bIdx = getCompFromID(COMP_BODY, WzString::fromUtf8(body));
        int pIdx = getCompFromID(COMP_PROPULSION, WzString::fromUtf8(prop));
        int wIdx = getCompFromID(COMP_WEAPON, WzString::fromUtf8(weap));
        if (bIdx < 0) bIdx = getCompFromID(COMP_BODY, WzString::fromUtf8("Body1REC"));
        if (pIdx < 0) pIdx = getCompFromID(COMP_PROPULSION, WzString::fromUtf8("wheeled01"));
        if (wIdx < 0) wIdx = getCompFromID(COMP_WEAPON, WzString::fromUtf8("MG1Mk1"));
        if (bIdx < 0 || pIdx < 0) return nullptr;

        auto tmpl = std::make_unique<DROID_TEMPLATE>();
        tmpl->name = WzString::fromUtf8(name);
        tmpl->id = WzString::fromUtf8(name);
        for (int c = 0; c < DROID_MAXCOMP; ++c) tmpl->asParts[c] = 0;
        tmpl->asParts[COMP_BODY] = bIdx;
        tmpl->asParts[COMP_PROPULSION] = pIdx;
        if (wIdx >= 0)
        {
            tmpl->numWeaps = 1;
            *(tmpl->asWeaps) = wIdx;
        }
        tmpl->droidType = droidTemplateType(tmpl.get());
        return addTemplate(static_cast<int>(player), std::move(tmpl));
    }

    DROID* spawnNativeDroid(const std::string& name, const std::string& body, const std::string& prop, const std::string& weap, int tileX, int tileY, UDWORD player)
    {
        DROID_TEMPLATE* psTempl = getOrRegisterTemplate(player, name, body, prop, weap);
        if (psTempl == nullptr) return nullptr;
        int wx = std::max(world_coord(4), std::min(world_coord(mapWidth - 5), world_coord(tileX)));
        int wy = std::max(world_coord(4), std::min(world_coord(mapHeight - 5), world_coord(tileY)));
        DROID* psNew = reallyBuildDroid(psTempl, Position(wx, wy, 0), player, false);
        if (psNew != nullptr) { addDroid(psNew, apsDroidLists); }
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
        int mapScavP = static_cast<int>(scavengerPlayer());
        if (mapScavP >= 0 && mapScavP < static_cast<int>(apsDroidLists.size()) && mapScavP != 1)
        {
            std::vector<DROID*> strayD(apsDroidLists.at(mapScavP).begin(), apsDroidLists.at(mapScavP).end());
            for (DROID* d : strayD) { if (d && !d->died) vanishDroid(d); }
            std::vector<STRUCTURE*> strayS(apsStructLists.at(mapScavP).begin(), apsStructLists.at(mapScavP).end());
            for (STRUCTURE* s : strayS) { if (s && !s->died) removeStruct(s, true); }
        }
        bool isScav = (target.owner == UsaFaction::SCAV_SYNDICATE);
        int enemyCol = (target.owner == UsaFaction::NEW_PARADIGM) ? 1 : ((target.owner == UsaFaction::THE_COLLECTIVE) ? 5 : ((target.owner == UsaFaction::NEXUS_REMNANT) ? 6 : 7));
        setPlayerColour(0, 0);
        setPlayerColour(1, enemyCol);
        setPower(1, isScav ? 15000 : 6000);

        int origP1X = std::max(16, std::min(mapWidth - 16, mapWidth - lzX));
        int origP1Y = std::max(16, std::min(mapHeight - 16, mapHeight - lzY));
        std::vector<STRUCTURE*> oldS(apsStructLists.at(1).begin(), apsStructLists.at(1).end());
        std::vector<DROID*> oldD(apsDroidLists.at(1).begin(), apsDroidLists.at(1).end());
        if (!oldS.empty() && oldS.front() != nullptr)
        {
            origP1X = map_coord(oldS.front()->pos.x);
            origP1Y = map_coord(oldS.front()->pos.y);
        }

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

            auto placeCitadelAt = [&](int cx, int cy) {
                for (const auto& item : scavLayout)
                {
                    spawnNativeStruct(item.id, cx + item.dx, cy + item.dy, 1);
                }
                spawnNativeDroid("Scav Raider", "B1BaBaPerson01", "BaBaLegs", "BaBaMG", cx + 1, cy + 3, 1);
                spawnNativeDroid("Scav Trike", "B4body-sml-trike01", "BaBaProp", "Rocket-Pod", cx + 3, cy + 3, 1);
                spawnNativeDroid("Scav Buggy", "B3body-sml-buggy01", "BaBaProp", "Flame1Mk1", cx + 5, cy + 3, 1);
            };

            // 1. Place primary citadel at Player 1's starting sector BEFORE removing old vanilla structures so entity count never hits 0!
            placeCitadelAt(origP1X, origP1Y);
            int placed = 1;

            // 2. Place up to 4 additional citadels across valid terrain
            for (int gx = 16; gx <= mapWidth - 18 && placed < 5; gx += 18)
            {
                for (int gy = 16; gy <= mapHeight - 18 && placed < 5; gy += 18)
                {
                    if (std::hypot(gx - lzX, gy - lzY) < 35.0) continue;
                    if (std::hypot(gx - origP1X, gy - origP1Y) < 24.0) continue;
                    if (terrainType(mapTile(gx, gy)) == TER_WATER || terrainType(mapTile(gx, gy)) == TER_CLIFFFACE) continue;
                    placeCitadelAt(gx, gy);
                    placed++;
                }
            }

            // 3. Now safely remove Player 1's old vanilla high-tech structures and droids
            for (STRUCTURE* s : oldS) { if (s && !s->died) removeStruct(s, true); }
            for (DROID* d : oldD) { if (d && !d->died) vanishDroid(d); }
        }
        else
        {
            spawnNativeStruct("A0PowerGenerator", origP1X - 4, origP1Y - 2, 1);
            spawnNativeStruct("A0PowerGenerator", origP1X - 4, origP1Y + 2, 1);
            spawnNativeStruct("A0LightFactory", origP1X + 3, origP1Y - 2, 1);
            spawnNativeStruct("A0LightFactory", origP1X + 3, origP1Y + 2, 1);
            spawnNativeStruct("A0ResearchFacility", origP1X, origP1Y - 4, 1);
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
