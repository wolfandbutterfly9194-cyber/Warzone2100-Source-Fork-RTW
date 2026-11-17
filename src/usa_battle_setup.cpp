#include "usa_battle_setup.h"
#include "usa_scav_gen.h"
#include "usa_campaign_state.h"
#include "lib/framework/frame.h"
#include "droid.h"
#include "feature.h"
#include "objmem.h"
#include "order.h"
#include "structure.h"
#include "research.h"
#include "template.h"
#include "component.h"
#include "map.h"
#include "multiplay.h"
#include "power.h"
#include "hci.h"
#include "fpath.h"
#include <algorithm>
#include <cmath>
#include <vector>

int usaBeachheadLzX = 24;
int usaBeachheadLzY = 24;

namespace
{
    STRUCTURE_STATS* findStructStats(const std::string& id)
    {
        for (UDWORD i = 0; i < numStructureStats; ++i)
        {
            if (asStructureStats[i].id.toUtf8() == id) return &asStructureStats[i];
        }
        return nullptr;
    }

    STRUCTURE* spawnExactStruct(const std::string& id, int tileX, int tileY, UDWORD player, uint16_t dir = 0)
    {
        STRUCTURE_STATS* psStats = findStructStats(id);
        if (psStats == nullptr || tileX < 5 || tileX >= mapWidth - 5 || tileY < 5 || tileY >= mapHeight - 5) return nullptr;
        int offX = (psStats->baseWidth % 2 == 0) ? 0 : (TILE_UNITS / 2);
        int offY = (psStats->baseBreadth % 2 == 0) ? 0 : (TILE_UNITS / 2);
        int wx = world_coord(tileX) + offX;
        int wy = world_coord(tileY) + offY;
        if (TileHasStructure(mapTile(tileX, tileY)) || !validLocation(psStats, Vector2i(wx, wy), dir, player, false)) return nullptr;
        STRUCTURE* built = buildStructureDir(psStats, static_cast<UDWORD>(wx), static_cast<UDWORD>(wy), dir, player, true);
        if (built != nullptr) { built->rot.direction = dir; built->status = SS_BUILT; built->body = built->structureBody(); }
        return built;
    }

    bool isFlatCompoundSite(int cx, int cy, int halfSpan)
    {
        if (cx - halfSpan < 6 || cx + halfSpan >= mapWidth - 6 || cy - halfSpan < 6 || cy + halfSpan >= mapHeight - 6) return false;
        int minH = 999999, maxH = -999999;
        for (int dx = -halfSpan; dx <= halfSpan; ++dx)
        {
            for (int dy = -halfSpan; dy <= halfSpan; ++dy)
            {
                MAPTILE* t = mapTile(cx + dx, cy + dy);
                if (!t || t->psObject != nullptr) return false;
                auto tt = terrainType(t);
                if (tt == TER_WATER || tt == TER_CLIFFFACE) return false;
                int h = map_TileHeight(cx + dx, cy + dy);
                minH = std::min(minH, h); maxH = std::max(maxH, h);
            }
        }
        return (maxH - minH) <= 14;
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
                    int tx = std::max(4, std::min(mapWidth - 5, tileX + dx));
                    int ty = std::max(4, std::min(mapHeight - 5, tileY + dy));
                    int wx = world_coord(tx) + TILE_UNITS / 2;
                    int wy = world_coord(ty) + TILE_UNITS / 2;
                    if (validLocation(psStats, Vector2i(wx, wy), 0, player, false))
                    {
                        STRUCTURE* built = buildStructureDir(psStats, static_cast<UDWORD>(wx), static_cast<UDWORD>(wy), 0, player, true);
                        if (built != nullptr) { built->status = SS_BUILT; built->body = built->structureBody(); }
                        return built;
                    }
                }
            }
        }
        return nullptr;
    }

    DROID_TEMPLATE* getOrRegisterTemplate(const std::string& name, const std::string& bodyId, const std::string& propId, const std::string& weaponId, const std::string& weapon2Id, bool isCommander, UDWORD player)
    {
        int bIdx = getCompFromID(COMP_BODY, WzString::fromUtf8(bodyId));
        if (bIdx < 0) bIdx = getCompFromID(COMP_BODY, WzString::fromUtf8("Body1REC"));
        int pIdx = getCompFromID(COMP_PROPULSION, WzString::fromUtf8(propId));
        if (pIdx < 0) pIdx = getCompFromID(COMP_PROPULSION, WzString::fromUtf8("wheeled01"));
        if (bIdx < 0 || pIdx < 0) return nullptr;
        auto tmpl = std::make_unique<DROID_TEMPLATE>();
        tmpl->name = WzString::fromUtf8(name);
        tmpl->id = WzString::fromUtf8(name);
        for (int i = 0; i < DROID_MAXCOMP; ++i) tmpl->asParts[i] = 0;
        tmpl->asParts[COMP_BODY] = bIdx;
        tmpl->asParts[COMP_PROPULSION] = pIdx;
        int w1Idx = weaponId.empty() ? -1 : getCompFromID(COMP_WEAPON, WzString::fromUtf8(weaponId));
        int w2Idx = weapon2Id.empty() ? -1 : getCompFromID(COMP_WEAPON, WzString::fromUtf8(weapon2Id));
        int cIdx = (w1Idx < 0 && !weaponId.empty()) ? getCompFromID(COMP_CONSTRUCT, WzString::fromUtf8(weaponId)) : -1;
        if (isCommander)
        {
            int brainIdx = getCompFromID(COMP_BRAIN, WzString::fromUtf8("CommandBrain01"));
            if (brainIdx >= 0) tmpl->asParts[COMP_BRAIN] = brainIdx;
        }
        if (w1Idx >= 0)
        {
            tmpl->numWeaps = (w2Idx >= 0) ? 2 : 1;
            *(tmpl->asWeaps) = w1Idx;
            if (w2Idx >= 0) *(tmpl->asWeaps + 1) = w2Idx;
        }
        else if (cIdx >= 0) { tmpl->asParts[COMP_CONSTRUCT] = cIdx; }
        else { tmpl->numWeaps = 0; }
        tmpl->droidType = isCommander ? DROID_COMMAND : (cIdx >= 0 ? DROID_CONSTRUCT : (bodyId.find("Transport") != std::string::npos ? DROID_SUPERTRANSPORTER : droidTemplateType(tmpl.get())));
        return addTemplate(player, std::move(tmpl));
    }

    DROID* spawnNativeDroid(const std::string& name, const std::string& bodyId, const std::string& propId, const std::string& weaponId, int tileX, int tileY, UDWORD player, const std::string& weapon2Id = "", bool isCommander = false)
    {
        DROID_TEMPLATE* psTmpl = getOrRegisterTemplate(name, bodyId, propId, weaponId, weapon2Id, isCommander, player);
        if (psTmpl == nullptr) return nullptr;
        int tx = std::max(4, std::min(mapWidth - 5, tileX));
        int ty = std::max(4, std::min(mapHeight - 5, tileY));
        int wx = world_coord(tx) + TILE_UNITS / 2;
        int wy = world_coord(ty) + TILE_UNITS / 2;
        DROID* psDroid = reallyBuildDroid(psTmpl, Position(wx, wy, 0), player, false);
        if (psDroid != nullptr)
        {
            if (psTmpl->droidType == DROID_CONSTRUCT) psDroid->droidType = DROID_CONSTRUCT;
            psDroid->baseSpeed = static_cast<UDWORD>(psDroid->baseSpeed * 1.10);
            psDroid->body = static_cast<UDWORD>(psDroid->body * 1.10);
            psDroid->originalBody = static_cast<UDWORD>(psDroid->originalBody * 1.10);
            addDroid(psDroid, apsDroidLists);
        }
        return psDroid;
    }

    void grantNativeResearch(const std::string& researchId, UDWORD player)
    {
        for (size_t i = 0; i < asResearch.size(); ++i)
        {
            if (asResearch[i].id.toUtf8() == researchId) { researchResult(i, player, false, nullptr, false); return; }
        }
    }
}
void usaApplyNativeBattleSetup()
{
    UsaCampaignState& camp = getUsaCampaignState();
    if (camp.activeTargetIndex < 0 || camp.activeTargetIndex >= static_cast<int>(camp.states.size())) return;
    Territory& target = camp.states[camp.activeTargetIndex];
    setPower(0, camp.power);
    int lzX = 16, lzY = 16;
    for (STRUCTURE* psS : apsStructLists.at(0))
    {
        if (psS && !psS->died) { lzX = map_coord(psS->pos.x); lzY = map_coord(psS->pos.y); break; }
    }
    usaBeachheadLzX = lzX;
    usaBeachheadLzY = lzY;

    if (!camp.activeSessionIsBaseMode)
    {
        int mapScavP = scavengerPlayer();
        if (mapScavP >= 0 && mapScavP < MAX_PLAYERS && mapScavP != 1)
        {
            std::vector<DROID*> strayD(apsDroidLists.at(mapScavP).begin(), apsDroidLists.at(mapScavP).end());
            for (DROID* d : strayD) { if (d && !d->died) vanishDroid(d); }
            std::vector<STRUCTURE*> strayS(apsStructLists.at(mapScavP).begin(), apsStructLists.at(mapScavP).end());
            for (STRUCTURE* s : strayS) { if (s && !s->died) removeStruct(s, true); }
        }
        bool isScav = isUsaScavFaction(target.owner);
        int enemyCol = (target.owner == UsaFaction::NEW_PARADIGM) ? 1 : 7;
        setPlayerColour(0, 0);
        setPlayerColour(1, enemyCol);
        setPower(1, isScav ? 15000 : 6000); auto unlockP1 = [](COMPONENT_TYPE t, size_t n) { if (apCompLists[1][t] != nullptr && n > 0) std::memset(apCompLists[1][t], AVAILABLE, n); }; unlockP1(COMP_BODY, asBodyStats.size()); unlockP1(COMP_PROPULSION, asPropulsionStats.size()); unlockP1(COMP_WEAPON, asWeaponStats.size()); unlockP1(COMP_SENSOR, asSensorStats.size()); unlockP1(COMP_ECM, asECMStats.size()); unlockP1(COMP_CONSTRUCT, asConstructStats.size()); unlockP1(COMP_REPAIRUNIT, asRepairStats.size()); unlockP1(COMP_BRAIN, asBrainStats.size());

        std::vector<std::pair<int, int>> valleyBases;
        for (int p = 1; p < MAX_PLAYERS; ++p)
        {
            if (p == scavengerPlayer()) continue;
            for (STRUCTURE* ps : apsStructLists.at(p))
            {
                if (ps && !ps->died && (ps->pStructureType->type == REF_HQ || ps->pStructureType->type == REF_FACTORY || ps->pStructureType->type == REF_POWER_GEN))
                {
                    int bx = map_coord(ps->pos.x), by = map_coord(ps->pos.y);
                    if (std::hypot(bx - lzX, by - lzY) >= 22.0) { valleyBases.push_back({ bx, by }); break; }
                }
            }
            if (p > 1)
            {
                std::vector<STRUCTURE*> extraS(apsStructLists.at(p).begin(), apsStructLists.at(p).end());
                for (STRUCTURE* s : extraS) { if (s && !s->died) removeStruct(s, true); }
                std::vector<DROID*> extraD(apsDroidLists.at(p).begin(), apsDroidLists.at(p).end());
                for (DROID* d : extraD) { if (d && !d->died) vanishDroid(d); }
            }
        }
        int origP1X = valleyBases.empty() ? std::max(16, std::min(mapWidth - 16, mapWidth - lzX)) : valleyBases.front().first;
        int origP1Y = valleyBases.empty() ? std::max(16, std::min(mapHeight - 16, mapHeight - lzY)) : valleyBases.front().second;
        std::vector<STRUCTURE*> oldS(apsStructLists.at(1).begin(), apsStructLists.at(1).end());
        std::vector<DROID*> oldD(apsDroidLists.at(1).begin(), apsDroidLists.at(1).end());

        if (isScav)
        {
            STRUCTURE* sentinel = spawnNativeStruct("A0BaBaBunker", 4, 4, 1);
            for (STRUCTURE* s : oldS) { if (s && !s->died) removeStruct(s, true); }
            for (DROID* d : oldD) { if (d && !d->died) vanishDroid(d); }
            ScavGen::generateAll(lzX, lzY, valleyBases);
            if (sentinel && !sentinel->died) removeStruct(sentinel, true);
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

    static const char* starterStructs[] = { "A0CommandCentre", "A0PowerGenerator", "A0ResourceExtractor", "A0LightFactory", "A0ResearchFacility", "A0RepairCentre3", "GuardTower1", "PillBox1", "TankTrapC", "A0HardcreteMk1Wall" };
    for (const char* sid : starterStructs)
    {
        for (UDWORD i = 0; i < numStructureStats; ++i)
        {
            if (asStructureStats[i].id.toUtf8() == sid)
            {
                apStructTypeLists[0][i] = AVAILABLE;
                asStructureStats[i].upgrade[0].limit = std::max<uint8_t>(10, asStructureStats[i].upgrade[0].limit);
                break;
            }
        }
    }
    for (const auto& t : camp.techs) { if (t.unlocked) grantNativeResearch(t.wzResearchId, 0); }

    if (!target.savedEntities.empty())
    {
        std::vector<STRUCTURE*> p0S(apsStructLists.at(0).begin(), apsStructLists.at(0).end());
        for (STRUCTURE* s : p0S) { if (s && !s->died) removeStruct(s, true); }
        std::vector<DROID*> p0D(apsDroidLists.at(0).begin(), apsDroidLists.at(0).end());
        for (DROID* d : p0D) { if (d && !d->died) vanishDroid(d); }
        for (const auto& ent : target.savedEntities)
        {
            if (ent.kind == "STRUCT") spawnNativeStruct(ent.id, ent.x, ent.y, 0);
            else if (ent.kind == "DROID") spawnNativeDroid(ent.name, ent.body, ent.prop, ent.weapon, ent.x, ent.y, 0);
        }
    }
    else if (camp.activeSessionIsBaseMode)
    {
        std::vector<STRUCTURE*> p0S(apsStructLists.at(0).begin(), apsStructLists.at(0).end());
        for (STRUCTURE* s : p0S) { if (s && !s->died) removeStruct(s, true); }
        std::vector<DROID*> p0D(apsDroidLists.at(0).begin(), apsDroidLists.at(0).end());
        for (DROID* d : p0D) { if (d && !d->died) vanishDroid(d); }
        spawnNativeDroid("Combat Engineer", "Body1REC", "wheeled01", "Spade1Mk1", lzX, lzY + 2, 0);
        spawnNativeDroid("Combat Engineer", "Body1REC", "wheeled01", "Spade1Mk1", lzX + 2, lzY + 2, 0);
    }
    if (!camp.activeSessionIsBaseMode && !camp.activeSessionIsDefense)
    {
        std::vector<STRUCTURE*> p0S(apsStructLists.at(0).begin(), apsStructLists.at(0).end());
        for (STRUCTURE* s : p0S) { if (s && !s->died) removeStruct(s, true); }
        std::vector<DROID*> p0D(apsDroidLists.at(0).begin(), apsDroidLists.at(0).end());
        for (DROID* d : p0D) { if (d && !d->died) vanishDroid(d); }
        std::string trBody = "TransportBody";
        for (size_t bi = 0; bi < asBodyStats.size(); ++bi) { if (asBodyStats[bi].id.toUtf8().find("Transport") != std::string::npos) { trBody = asBodyStats[bi].id.toUtf8(); break; } }
        int edgeX = usaBeachheadLzX, edgeY = 6;
        double bestEdgeDist = 999999.0;
        for (int bx = 6; bx < mapWidth - 6; ++bx)
        {
            for (int side = 0; side < 2; ++side) { int by = (side == 0) ? 6 : (mapHeight - 7); if (!fpathBlockingTile(bx, by, PROPULSION_TYPE_WHEELED)) { double d = std::hypot(bx - usaBeachheadLzX, by - usaBeachheadLzY); if (d >= 10.0 && d <= 24.0 && d < bestEdgeDist) { bestEdgeDist = d; edgeX = bx; edgeY = by; } } }
        }
        for (int by = 6; by < mapHeight - 6; ++by)
        {
            for (int side = 0; side < 2; ++side) { int bx = (side == 0) ? 6 : (mapWidth - 7); if (!fpathBlockingTile(bx, by, PROPULSION_TYPE_WHEELED)) { double d = std::hypot(bx - usaBeachheadLzX, by - usaBeachheadLzY); if (d >= 10.0 && d <= 24.0 && d < bestEdgeDist) { bestEdgeDist = d; edgeX = bx; edgeY = by; } } }
        }
        DROID* ship = spawnNativeDroid("Invasion Dropship", trBody, "V-Tol", "", edgeX, edgeY, 0);
        if (ship != nullptr)
        {
            ship->pos.z = map_Height(ship->pos.x, ship->pos.y) + 450;
            ship->sMove.Status = MOVEINACTIVE;
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
        target.savedEntities.clear();
        std::vector<int> keepOwners = { 0 };
        if (!camp.activeSessionIsBaseMode && playerWon) keepOwners.push_back(1);
        for (int ow : keepOwners)
        {
            for (STRUCTURE* psS : apsStructLists.at(ow))
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
        }
        for (DROID* psD : apsDroidLists.at(0))
        {
            if (!psD || psD->died || psD->droidType == DROID_COMMAND || psD->droidType == DROID_TRANSPORTER || psD->droidType == DROID_SUPERTRANSPORTER) continue;
            SavedEntity e;
            e.kind = "DROID";
            e.name = psD->aName;
            e.body = (psD->asBits[COMP_BODY] < asBodyStats.size()) ? asBodyStats[psD->asBits[COMP_BODY]].id.toUtf8() : "Body1REC";
            e.prop = (psD->asBits[COMP_PROPULSION] < asPropulsionStats.size()) ? asPropulsionStats[psD->asBits[COMP_PROPULSION]].id.toUtf8() : "wheeled01";
            e.weapon = (psD->numWeaps > 0 && psD->asWeaps[0].nStat < asWeaponStats.size()) ? asWeaponStats[psD->asWeaps[0].nStat].id.toUtf8() : (psD->droidType == DROID_CONSTRUCT ? "Spade1Mk1" : "MG1Mk1");
            e.x = map_coord(psD->pos.x);
            e.y = map_coord(psD->pos.y);
            target.savedEntities.push_back(e);
        }
        if (!camp.activeSessionIsBaseMode && playerWon) { target.owner = UsaFaction::THE_PROJECT; camp.power += 500; }
    }
    camp.activeSessionIsBaseMode = false;
    camp.activeSessionIsDefense = false;
    camp.saveToPhysfs();
}

