#include "usa_scav_gen.h"
#include "lib/framework/frame.h"
#include "droid.h"
#include "feature.h"
#include "fpath.h"
#include "map.h"
#include "objmem.h"
#include "order.h"
#include "structure.h"
#include "template.h"
#include "component.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace
{
    STRUCTURE_STATS* findStats(const std::string& id)
    {
        for (UDWORD i = 0; i < numStructureStats; ++i)
            if (asStructureStats[i].id.toUtf8() == id) return &asStructureStats[i];
        return nullptr;
    }

    int localSlope(int cx, int cy, int rad)
    {
        int minH = 999999, maxH = -999999;
        for (int dx = -rad; dx <= rad; ++dx)
            for (int dy = -rad; dy <= rad; ++dy)
            {
                int tx = std::max(2, std::min(mapWidth - 3, cx + dx));
                int ty = std::max(2, std::min(mapHeight - 3, cy + dy));
                int h = map_TileHeight(tx, ty);
                minH = std::min(minH, h); maxH = std::max(maxH, h);
            }
        return maxH - minH;
    }

    bool canPlaceFootprint(int tx, int ty, int w, int b, int pad, int maxSlope)
    {
        if (tx < 5 || tx + w >= mapWidth - 5 || ty < 5 || ty + b >= mapHeight - 5) return false;
        for (int dx = -pad; dx < w + pad; ++dx)
            for (int dy = -pad; dy < b + pad; ++dy)
            {
                MAPTILE* t = mapTile(tx + dx, ty + dy);
                if (!t || TileHasStructure(t) || fpathBlockingTile(tx + dx, ty + dy, PROPULSION_TYPE_WHEELED)) return false;
            }
        return localSlope(tx + w / 2, ty + b / 2, std::max(1, std::max(w, b))) <= maxSlope;
    }

    STRUCTURE* placeWallTile(const std::string& id, int tileX, int tileY, uint16_t dir)
    {
        STRUCTURE_STATS* psStats = findStats(id);
        if (!psStats || tileX < 5 || tileX >= mapWidth - 5 || tileY < 5 || tileY >= mapHeight - 5) return nullptr;
        MAPTILE* t = mapTile(tileX, tileY);
        if (!t || TileHasStructure(t) || fpathBlockingTile(tileX, tileY, PROPULSION_TYPE_WHEELED)) return nullptr;
        int wx = world_coord(tileX) + TILE_UNITS / 2, wy = world_coord(tileY) + TILE_UNITS / 2;
        if (!validLocation(psStats, Vector2i(wx, wy), dir, 1, false)) return nullptr;
        STRUCTURE* s = buildStructureDir(psStats, static_cast<UDWORD>(wx), static_cast<UDWORD>(wy), dir, 1, true);
        if (s) { s->rot.direction = dir; s->status = SS_BUILT; s->body = s->structureBody(); }
        return s;
    }
    STRUCTURE* placeInsideBox(const std::string& id, int prefX, int prefY, int minX, int maxX, int minY, int maxY)
    {
        STRUCTURE_STATS* psStats = findStats(id);
        if (!psStats) return nullptr;
        int w = static_cast<int>(psStats->baseWidth), b = static_cast<int>(psStats->baseBreadth);
        int bestX = -1, bestY = -1, bestDist = 999999;
        for (int tx = minX; tx <= maxX - w + 1; ++tx)
            for (int ty = minY; ty <= maxY - b + 1; ++ty)
            {
                if (!canPlaceFootprint(tx, ty, w, b, 1, 55)) continue;
                int d = std::abs(tx - prefX) + std::abs(ty - prefY);
                if (d < bestDist) { bestDist = d; bestX = tx; bestY = ty; }
            }
        if (bestX < 0) return nullptr;
        int wx = world_coord(bestX) + ((w % 2 == 0) ? 0 : (TILE_UNITS / 2));
        int wy = world_coord(bestY) + ((b % 2 == 0) ? 0 : (TILE_UNITS / 2));
        if (!validLocation(psStats, Vector2i(wx, wy), 0, 1, false)) return nullptr;
        STRUCTURE* s = buildStructureDir(psStats, static_cast<UDWORD>(wx), static_cast<UDWORD>(wy), 0, 1, true);
        if (s) { s->status = SS_BUILT; s->body = s->structureBody(); }
        return s;
    }

    void spawnGuard(const char* name, const char* bodyId, const char* propId, const char* weapId, int cx, int cy)
    {
        int bIdx = getCompFromID(COMP_BODY, WzString::fromUtf8(bodyId));
        int pIdx = getCompFromID(COMP_PROPULSION, WzString::fromUtf8(propId));
        int wIdx = getCompFromID(COMP_WEAPON, WzString::fromUtf8(weapId));
        if (bIdx < 0 || pIdx < 0 || wIdx < 0) return;
        for (int r = 0; r <= 6; ++r)
            for (int dx = -r; dx <= r; ++dx)
                for (int dy = -r; dy <= r; ++dy)
                {
                    int tx = std::max(5, std::min(mapWidth - 6, cx + dx)), ty = std::max(5, std::min(mapHeight - 6, cy + dy));
                    if (fpathBlockingTile(tx, ty, PROPULSION_TYPE_WHEELED) || TileHasStructure(mapTile(tx, ty))) continue;
                    auto tmpl = std::make_unique<DROID_TEMPLATE>();
                    tmpl->name = WzString::fromUtf8(name); tmpl->id = WzString::fromUtf8(name);
                    for (int k = 0; k < DROID_MAXCOMP; ++k) tmpl->asParts[k] = 0;
                    tmpl->asParts[COMP_BODY] = bIdx; tmpl->asParts[COMP_PROPULSION] = pIdx;
                    tmpl->numWeaps = 1; *(tmpl->asWeaps) = wIdx; tmpl->droidType = DROID_WEAPON;
                    DROID_TEMPLATE* pt = addTemplate(1, std::move(tmpl));
                    if (!pt) return;
                    DROID* d = reallyBuildDroid(pt, Position(world_coord(tx) + TILE_UNITS / 2, world_coord(ty) + TILE_UNITS / 2, 0), 1, false);
                    if (d) addDroid(d, apsDroidLists);
                    return;
                }
    }
}
namespace ScavGen
{
    void buildCompound(int cx, int cy, bool majorFortress)
    {
        int r = majorFortress ? 7 : 6;
        cx = std::max(r + 6, std::min(mapWidth - r - 7, cx));
        cy = std::max(r + 6, std::min(mapHeight - r - 7, cy));

        placeWallTile("A0BabaCornerWall", cx - r, cy - r, 16384);
        placeWallTile("A0BabaCornerWall", cx + r, cy - r, 32768);
        placeWallTile("A0BabaCornerWall", cx - r, cy + r, 0);
        placeWallTile("A0BabaCornerWall", cx + r, cy + r, 49152);
        for (int x = -r + 1; x <= r - 1; ++x)
        {
            placeWallTile("A0BaBaHorizontalWall", cx + x, cy - r, 0);
            if (std::abs(x) >= 2) placeWallTile("A0BaBaHorizontalWall", cx + x, cy + r, 0);
        }
        for (int y = -r + 1; y <= r - 1; ++y)
        {
            placeWallTile("A0BaBaHorizontalWall", cx - r, cy + y, 16384);
            placeWallTile("A0BaBaHorizontalWall", cx + r, cy + y, 16384);
        }

        int minX = cx - r + 2, maxX = cx + r - 2, minY = cy - r + 2, maxY = cy + r - 2;
        placeInsideBox("A0BaBaFactory", cx - 3, cy - 3, minX, maxX, minY, maxY);
        placeInsideBox("A0BaBaFactory", cx + 3, cy - 3, minX, maxX, minY, maxY);
        placeInsideBox("A0BaBaPowerGenerator", cx, cy - 4, minX, maxX, minY, maxY);
        placeInsideBox("A0BaBaPowerGenerator", cx, cy - 2, minX, maxX, minY, maxY);
        placeInsideBox("Sys-SensoTower01", cx, cy, minX, maxX, minY, maxY);
        placeInsideBox("ScavRepairCentre", cx, cy + 2, minX, maxX, minY, maxY);
        placeInsideBox("A0BaBaMortarPit", cx - 3, cy + 1, minX, maxX, minY, maxY);
        placeInsideBox("A0BaBaRocketPitAT", cx + 3, cy + 1, minX, maxX, minY, maxY);
        if (majorFortress)
        {
            placeInsideBox("A0BaBaVtolFactory", cx - 3, cy + 3, minX, maxX, minY, maxY);
            placeInsideBox("A0BaBaVtolPad", cx + 3, cy + 3, minX, maxX, minY, maxY);
        }

        placeInsideBox("A0BaBaBunker", cx - 3, cy + r + 2, cx - r, cx + r, cy + r + 1, cy + r + 4);
        placeInsideBox("A0BaBaBunker", cx + 3, cy + r + 2, cx - r, cx + r, cy + r + 1, cy + r + 4);
        placeInsideBox("A0BaBaFlameTower", cx - 2, cy + r + 1, cx - r, cx + r, cy + r + 1, cy + r + 3);
        placeInsideBox("A0BaBaFlameTower", cx + 2, cy + r + 1, cx - r, cx + r, cy + r + 1, cy + r + 3);
    }

    void fortifyOilNodes(int lzX, int lzY)
    {
        STRUCTURE_STATS* extStats = findStats("A0ResourceExtractor");
        if (!extStats) return;
        for (int otx = 4; otx < mapWidth - 4; ++otx)
            for (int oty = 4; oty < mapHeight - 4; ++oty)
            {
                FEATURE* psFeat = dynamic_cast<FEATURE*>(mapTile(otx, oty)->psObject);
                if (!psFeat || psFeat->died || !psFeat->psStats || psFeat->psStats->subType != FEAT_OIL_RESOURCE) continue;
                if (std::hypot(otx - lzX, oty - lzY) < 18.0) continue;
                STRUCTURE* derrick = buildStructureDir(extStats, psFeat->pos.x, psFeat->pos.y, 0, 1, true);
                if (!derrick) continue;
                derrick->status = SS_BUILT; derrick->body = derrick->structureBody();
                STRUCTURE* pGen = placeInsideBox("A0BaBaPowerGenerator", otx + 2, oty, otx - 4, otx + 5, oty - 4, oty + 5);
                if (pGen && derrick->pFunctionality) derrick->pFunctionality->resourceExtractor.psPowerGen = pGen;
                placeInsideBox("A0BaBaBunker", otx - 2, oty, otx - 4, otx + 4, oty - 4, oty + 4);
                placeInsideBox("A0BaBaRocketPitAT", otx, oty + 2, otx - 4, otx + 4, oty - 4, oty + 4);
                placeInsideBox("A0BaBaFlameTower", otx, oty - 2, otx - 4, otx + 4, oty - 4, oty + 4);
                spawnGuard("Scav Oil Guard", "B2JeepBody", "BaBaProp", "Cannon1Mk1", otx + 1, oty + 2);
                spawnGuard("Scav Oil Trike", "B4body-sml-trike01", "BaBaProp", "Rocket-Pod", otx - 1, oty + 2);
            }
    }

    void generateAll(int beachheadLzX, int beachheadLzY, const std::vector<std::pair<int, int>>& seedValleys)
    {
        (void)seedValleys;
        std::vector<std::pair<int, int>> bases;
        int qX[4] = { mapWidth / 4, (mapWidth * 3) / 4, mapWidth / 4, (mapWidth * 3) / 4 };
        int qY[4] = { mapHeight / 4, mapHeight / 4, (mapHeight * 3) / 4, (mapHeight * 3) / 4 };
        for (int q = 0; q < 4; ++q)
        {
            if (std::hypot(qX[q] - beachheadLzX, qY[q] - beachheadLzY) < 26.0) continue;
            int minQx = (q % 2 == 0) ? 14 : (mapWidth / 2 + 4), maxQx = (q % 2 == 0) ? (mapWidth / 2 - 4) : (mapWidth - 15);
            int minQy = (q < 2) ? 14 : (mapHeight / 2 + 4), maxQy = (q < 2) ? (mapHeight / 2 - 4) : (mapHeight - 15);
            int bestX = qX[q], bestY = qY[q], bestScore = -999999;
            for (int cx = minQx; cx <= maxQx; cx += 2)
                for (int cy = minQy; cy <= maxQy; cy += 2)
                {
                    if (std::hypot(cx - beachheadLzX, cy - beachheadLzY) < 26.0) continue;
                    int openCount = 0;
                    for (int dx = -6; dx <= 6; ++dx)
                        for (int dy = -6; dy <= 6; ++dy)
                            if (!fpathBlockingTile(cx + dx, cy + dy, PROPULSION_TYPE_WHEELED) && !TileHasStructure(mapTile(cx + dx, cy + dy))) openCount++;
                    int score = openCount * 10 - localSlope(cx, cy, 6) * 2;
                    if (score > bestScore) { bestScore = score; bestX = cx; bestY = cy; }
                }
            bases.push_back({ bestX, bestY });
        }
        for (size_t i = 0; i < bases.size(); ++i) buildCompound(bases[i].first, bases[i].second, i == 0);
        fortifyOilNodes(beachheadLzX, beachheadLzY);
    }
}
