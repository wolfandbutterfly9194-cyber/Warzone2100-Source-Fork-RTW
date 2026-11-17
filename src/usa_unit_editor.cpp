#include "usa_unit_editor.h"
#include "usa_campaign_state.h"
#include "lib/framework/frame.h"
#include "lib/framework/input.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/piematrix.h"
#include "lib/ivis_opengl/piedef.h"
#include "lib/ivis_opengl/textdraw.h"
#include "lib/widget/widget.h"
#include "component.h"
#include "droid.h"
#include "hci.h"
#include "intdisplay.h"
#include "stats.h"
#include "template.h"
#include "wrappers.h"
#include "frontend.h"
#include "multiplay.h"
#include "titleui/multiplayer.h"
#include <physfs.h>
extern bool bUsaPrebattle;
extern bool bUsaStripVanillaLobby;
extern bool bUsaSessionActive;
extern std::string challengeName;
extern void usaSetSkirmishTestName(const std::string& name);

bool bUsaSimulatorMode = false;
std::string usaSimulatorClassName = "Dragon-X Behemoth";
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>

extern bool usaEnsureOverworldDesignAssets();

namespace
{
    bool editorOpen = false;
    std::vector<SuperHeavyBlueprint> blueprints;
    int selectedBpIdx = 0, activeTurretSlot = 0;
    float previewYaw = 35.0f, previewPitch = -22.0f, previewZoom = 1.0f;
    bool autoSpin = false, renamingClass = false, isDraggingView = false;
    int lastDragX = 0, lastDragY = 0;
    std::string statusBanner = "READY - L/R-CLICK PARTS, RENAME CLASS, OR CLICK YELLOW HULL NODES";
    struct ButtonBox { int id, x, y, w, h; };
    std::vector<ButtonBox> clickBoxes;
    const char* kBodies[] = { "Body12SUP", "Body14SUP", "Body11ABT", "Body9REC", "Body10MBT", "Body6SUPP", "Body7ABT", "Body5REC", "Body1REC" };
    const char* kBodyNames[] = { "Dragon Super", "Wyvern Super", "Python Heavy", "Tiger Super", "Vengeance Super", "Panther Med", "Retribution", "Cobra Med", "Viper Light" };
    const char* kProps[] = { "tracked01", "HalfTrack", "wheeled01", "hover01", "V-Tol" };
    const char* kPropNames[] = { "Heavy Tracks", "Half-Tracks", "Heavy Wheels", "Hover Skirt", "Heavy VTOL" };
    const char* kWeapons[] = { "Cannon375mmMk1", "Cannon2A-TMk1", "Rocket-HvyA-T", "Missile-A-T", "RailGun3Mk1", "Laser2PULSEMk1", "Howitzer105Mk1", "Rocket-Pod", "MG3Mk1" };
    const char* kWeapNames[] = { "Heavy Cannon", "Medium Cannon", "Tank Killer", "Scourge AT", "Gauss Cannon", "Pulse Laser", "Howitzer", "Mini-Rocket", "Heavy MG" };
    const char* kPresetNames[] = { "Roof Center", "Forward Deck", "Rear Deck", "Left Sponson", "Right Sponson", "High Command" };
    void initDefaultBlueprints()
    {
        if (!blueprints.empty()) return;
        SuperHeavyBlueprint m;
        m.name = "Mammoth Behemoth"; m.bodyId = "Body11ABT"; m.propId = "tracked01";
        m.bodyScaleX = 1.75f; m.bodyScaleY = 1.60f; m.bodyScaleZ = 1.85f; m.bodyOffsetY = 9.0f;
        m.propScaleX = 1.75f; m.propScaleY = 1.50f; m.propScaleZ = 1.85f; m.numTurrets = 2;
        m.turrets[0] = { "Cannon375mmMk1", 1.35f, 0.0f, 10.0f, 14.0f, 1 };
        m.turrets[1] = { "Rocket-HvyA-T", 1.25f, 0.0f, 14.0f, -16.0f, 2 };
        blueprints.push_back(m);

        SuperHeavyBlueprint l;
        l.name = "Leviathan Dreadnought"; l.bodyId = "Body10MBT"; l.propId = "tracked01";
        l.bodyScaleX = 2.05f; l.bodyScaleY = 1.80f; l.bodyScaleZ = 2.15f; l.bodyOffsetY = 12.0f;
        l.propScaleX = 2.05f; l.propScaleY = 1.65f; l.propScaleZ = 2.15f; l.numTurrets = 3;
        l.turrets[0] = { "RailGun3Mk1", 1.45f, 0.0f, 16.0f, 8.0f, 0 };
        l.turrets[1] = { "Missile-A-T", 1.20f, -22.0f, 8.0f, -6.0f, 3 };
        l.turrets[2] = { "Missile-A-T", 1.20f, 22.0f, 8.0f, -6.0f, 4 };
        blueprints.push_back(l);
    }
}
const std::vector<SuperHeavyBlueprint>& usaGetSuperHeavyBlueprints() { initDefaultBlueprints(); return blueprints; }
const SuperHeavyBlueprint* usaFindSuperHeavyByName(const std::string& name)
{
    initDefaultBlueprints();
    for (const auto& bp : blueprints) if (bp.name == name) return &bp;
    return nullptr;
}
void usaSaveSuperHeavyBlueprints()
{
    initDefaultBlueprints();
    std::ofstream out("usa_superheavy_units.json", std::ios::binary);
    if (!out.is_open()) return;
    out << "{\n  \"units\": [\n";
    for (size_t i = 0; i < blueprints.size(); ++i)
    {
        const auto& u = blueprints[i];
        out << "    {\"name\":\"" << u.name << "\",\"bodyId\":\"" << u.bodyId << "\",\"propId\":\"" << u.propId << "\","
            << "\"bodyScale\":[" << u.bodyScaleX << "," << u.bodyScaleY << "," << u.bodyScaleZ << "],\"bodyOff\":[" << u.bodyOffsetY << "," << u.bodyOffsetZ << "],"
            << "\"propScale\":[" << u.propScaleX << "," << u.propScaleY << "," << u.propScaleZ << "],\"propOff\":[" << u.propOffsetY << "," << u.propOffsetZ << "],"
            << "\"numTurrets\":" << u.numTurrets << ",\"turrets\":[";
        for (int t = 0; t < 4; ++t)
            out << "{\"w\":\"" << u.turrets[t].weaponId << "\",\"s\":" << u.turrets[t].scale << ",\"x\":" << u.turrets[t].snapX << ",\"y\":" << u.turrets[t].snapY << ",\"z\":" << u.turrets[t].snapZ << ",\"p\":" << u.turrets[t].presetIndex << "}" << (t < 3 ? "," : "");
        out << "]}" << (i + 1 < blueprints.size() ? "," : "") << "\n";
    }
    out << "  ]\n}\n";
    statusBanner = "SAVED " + std::to_string(blueprints.size()) + " SUPER-HEAVY BLUEPRINTS";
}
void usaLoadSuperHeavyBlueprints()
{
    initDefaultBlueprints();
    std::ifstream in("usa_superheavy_units.json", std::ios::binary);
    if (!in.is_open()) return;
    std::stringstream ss; ss << in.rdbuf(); std::string raw = ss.str();
    std::vector<SuperHeavyBlueprint> loaded; size_t pos = 0;
    while ((pos = raw.find("\"name\":", pos)) != std::string::npos)
    {
        SuperHeavyBlueprint u;
        auto readStr = [&](const std::string& key, size_t from) -> std::string {
            size_t k = raw.find(key, from); if (k == std::string::npos) return "";
            size_t q1 = raw.find('"', k + key.size()), q2 = (q1 != std::string::npos) ? raw.find('"', q1 + 1) : std::string::npos;
            return (q1 != std::string::npos && q2 != std::string::npos) ? raw.substr(q1 + 1, q2 - q1 - 1) : "";
        };
        u.name = readStr("\"name\":", pos); u.bodyId = readStr("\"bodyId\":", pos); u.propId = readStr("\"propId\":", pos);
        size_t bs = raw.find("\"bodyScale\":", pos); if (bs != std::string::npos) std::sscanf(raw.c_str() + bs + 12, "[%f,%f,%f]", &u.bodyScaleX, &u.bodyScaleY, &u.bodyScaleZ);
        size_t bo = raw.find("\"bodyOff\":", pos); if (bo != std::string::npos) std::sscanf(raw.c_str() + bo + 10, "[%f,%f]", &u.bodyOffsetY, &u.bodyOffsetZ);
        size_t ps = raw.find("\"propScale\":", pos); if (ps != std::string::npos) std::sscanf(raw.c_str() + ps + 12, "[%f,%f,%f]", &u.propScaleX, &u.propScaleY, &u.propScaleZ);
        size_t po = raw.find("\"propOff\":", pos); if (po != std::string::npos) std::sscanf(raw.c_str() + po + 10, "[%f,%f]", &u.propOffsetY, &u.propOffsetZ);
        size_t nt = raw.find("\"numTurrets\":", pos); if (nt != std::string::npos) std::sscanf(raw.c_str() + nt + 13, "%d", &u.numTurrets);
        size_t tp = raw.find("\"turrets\":", pos);
        for (int t = 0; t < 4 && tp != std::string::npos; ++t)
        {
            size_t ob = raw.find('{', tp); if (ob == std::string::npos) break;
            u.turrets[t].weaponId = readStr("\"w\":", ob);
            size_t sp = raw.find("\"s\":", ob); if (sp != std::string::npos) std::sscanf(raw.c_str() + sp + 4, "%f,\"x\":%f,\"y\":%f,\"z\":%f,\"p\":%d", &u.turrets[t].scale, &u.turrets[t].snapX, &u.turrets[t].snapY, &u.turrets[t].snapZ, &u.turrets[t].presetIndex);
            tp = ob + 1;
        }
        for (int t = 0; t < 4; ++t) if (u.turrets[t].weaponId.empty()) u.turrets[t].weaponId = kWeapons[t % 9];
        if (!u.name.empty() && !u.bodyId.empty()) loaded.push_back(u);
        pos += 8;
    }
    if (!loaded.empty()) blueprints = std::move(loaded);
}
void usaRegisterSuperHeavyTemplatesForPlayer(int player)
{
    usaLoadSuperHeavyBlueprints();
    for (const auto& u : blueprints)
    {
        int bIdx = getCompFromID(COMP_BODY, WzString::fromUtf8(u.bodyId)), pIdx = getCompFromID(COMP_PROPULSION, WzString::fromUtf8(u.propId));
        if (bIdx < 0 || pIdx < 0) continue;
        auto tmpl = std::make_unique<DROID_TEMPLATE>();
        tmpl->name = WzString::fromUtf8(u.name); tmpl->id = WzString::fromUtf8(u.name);
        for (int k = 0; k < DROID_MAXCOMP; ++k) tmpl->asParts[k] = 0;
        tmpl->asParts[COMP_BODY] = bIdx; tmpl->asParts[COMP_PROPULSION] = pIdx;
        int wCount = 0;
        for (int t = 0; t < std::min(4, u.numTurrets); ++t) { int wIdx = getCompFromID(COMP_WEAPON, WzString::fromUtf8(u.turrets[t].weaponId)); if (wIdx >= 0) { *(tmpl->asWeaps + wCount) = wIdx; wCount++; } }
        tmpl->numWeaps = wCount; tmpl->droidType = DROID_WEAPON; addTemplate(player, std::move(tmpl));
    }
}
namespace
{
    void applyTurretPreset(SuperHeavyBlueprint& u, int slot)
    {
        auto& tr = u.turrets[slot];
        float hx = 14.0f * u.bodyScaleX, hy = 10.0f * u.bodyScaleY, hz = 16.0f * u.bodyScaleZ;
        switch (tr.presetIndex % 6)
        {
        case 0: tr.snapX = 0.0f; tr.snapY = hy;        tr.snapZ = 0.0f;  break;
        case 1: tr.snapX = 0.0f; tr.snapY = hy * 0.8f; tr.snapZ = hz;    break;
        case 2: tr.snapX = 0.0f; tr.snapY = hy * 1.1f; tr.snapZ = -hz;   break;
        case 3: tr.snapX = -hx;  tr.snapY = hy * 0.5f; tr.snapZ = 0.0f;  break;
        case 4: tr.snapX = hx;   tr.snapY = hy * 0.5f; tr.snapZ = 0.0f;  break;
        case 5: tr.snapX = 0.0f; tr.snapY = hy * 1.5f; tr.snapZ = -4.0f; break;
        }
    }
    void drawBtn(int id, int x, int y, int w, int h, const std::string& label, PIELIGHT fill, PIELIGHT border)
    {
        clickBoxes.push_back({ id, x, y, w, h });
        pie_UniTransBoxFill(x, y, x + w, y + h, fill); iV_Box(x, y, x + w, y + h, border);
        iV_SetTextColour(WZCOL_WHITE); int tw = iV_GetTextWidth(label.c_str(), font_regular);
        iV_DrawText(label.c_str(), x + std::max(6, (w - tw) / 2), y + (h / 2) + 4, font_regular);
    }
    struct HullNodeSpec { const char* label; float nx, ny, nz; };
    const HullNodeSpec kHullNodes[] = {
        { "ROOF CENTER", 0.0f, 1.05f, 0.0f }, { "FORWARD DECK", 0.0f, 0.82f, 1.05f }, { "REAR DECK", 0.0f, 1.12f, -1.05f },
        { "PORT SPONSON", -1.05f, 0.55f, 0.05f }, { "STBD SPONSON", 1.05f, 0.55f, 0.05f },
        { "PORT CHEEK", -0.68f, 0.72f, 0.85f }, { "STBD CHEEK", 0.68f, 0.72f, 0.85f },
        { "AFT PORT", -0.75f, 0.85f, -0.82f }, { "AFT STBD", 0.75f, 0.85f, -0.82f }
    };
    void draw3DPreview(const SuperHeavyBlueprint& u, int cx, int cy, int boxW, int boxH)
    {
        if (mouseDown(MOUSE_RMB)) { int mx = mouseX(), my = mouseY(); if (!isDraggingView && mx >= cx - boxW / 2 && mx <= cx + boxW / 2 && my >= cy - boxH / 2 && my <= cy + boxH / 2) { isDraggingView = true; lastDragX = mx; lastDragY = my; } if (isDraggingView) { autoSpin = false; previewYaw = std::fmod(previewYaw + (mx - lastDragX) * 0.65f + 360.0f, 360.0f); previewPitch = std::max(-75.0f, std::min(15.0f, previewPitch + (my - lastDragY) * 0.55f)); lastDragX = mx; lastDragY = my; } } else { isDraggingView = false; }
        iV_Box(cx - boxW / 2, cy - boxH / 2, cx + boxW / 2, cy + boxH / 2, pal_RGBA(70, 175, 255, 255));
        if (autoSpin) previewYaw = std::fmod(previewYaw + 0.55f, 360.0f);
        for (int g = -4; g <= 4; ++g)
            iV_Line(cx + g * 28, cy + boxH / 2 - 42, cx + g * 36, cy + boxH / 2 - 12, pal_RGBA(35, 95, 140, 160));
        int baseCy = cy + 24;
        int bIdx = getCompFromID(COMP_BODY, WzString::fromUtf8(u.bodyId));
        int pIdx = getCompFromID(COMP_PROPULSION, WzString::fromUtf8(u.propId));
        if (bIdx >= 0 && pIdx >= 0)
        {
            pie_Begin3DScene(); pie_SetGeometricOffset(cx, baseCy);
            float sc = (DROID_BUT_SCALE * previewZoom * 1.65f) / 100.0f;
            glm::mat4 rootMat = glm::translate(glm::vec3(0.f, 0.f, 680.f)) * glm::rotate(previewPitch * 3.14159265f / 180.f, glm::vec3(1.f, 0.f, 0.f)) * glm::rotate(previewYaw * 3.14159265f / 180.f, glm::vec3(0.f, 1.f, 0.f)) * glm::scale(glm::vec3(sc));
            auto drawPart3D = [&](const iIMDBaseShape* baseSh, const glm::mat4& mat) {
                if (!baseSh) return;
                for (const iIMDShape* imd = baseSh->displayModel(); imd != nullptr; imd = imd->next.get())
                    pie_Draw3DButton(imd, imd->getTeamColourForModel(getPlayerColour(0)), mat, glm::mat4(1.f));
            };
            drawPart3D(asPropulsionStats[pIdx].pIMD, rootMat * glm::translate(glm::vec3(0.f, u.propOffsetY, u.propOffsetZ)) * glm::scale(glm::vec3(u.propScaleX, u.propScaleY, u.propScaleZ)));
            drawPart3D(asBodyStats[bIdx].pIMD, rootMat * glm::translate(glm::vec3(0.f, u.bodyOffsetY, u.bodyOffsetZ)) * glm::scale(glm::vec3(u.bodyScaleX, u.bodyScaleY, u.bodyScaleZ)));
            for (int t = 0; t < std::min(4, u.numTurrets); ++t)
            {
                int wIdx = getCompFromID(COMP_WEAPON, WzString::fromUtf8(u.turrets[t].weaponId));
                if (wIdx >= 0)
                {
                    glm::mat4 tMat = rootMat * glm::translate(glm::vec3(u.turrets[t].snapX, u.bodyOffsetY + u.turrets[t].snapY, u.bodyOffsetZ + u.turrets[t].snapZ)) * glm::scale(glm::vec3(u.turrets[t].scale));
                    drawPart3D(asWeaponStats[wIdx].pMountGraphic, tMat); drawPart3D(asWeaponStats[wIdx].pIMD, tMat);
                }
            }
            pie_BeginInterface(); pie_SetGeometricOffset(pie_GetVideoBufferWidth() / 2, pie_GetVideoBufferHeight() / 2);
        }
        float yawRad = previewYaw * 3.14159265f / 180.0f, pitchRad = -previewPitch * 3.14159265f / 180.0f;
        float cosY = std::cos(yawRad), sinY = std::sin(yawRad), cosP = std::cos(pitchRad), sinP = std::sin(pitchRad), pxScale = 2.45f * previewZoom;
        for (int n = 0; n < 9; ++n)
        {
            float hx = kHullNodes[n].nx * 15.0f * u.bodyScaleX, hy = kHullNodes[n].ny * 11.0f * u.bodyScaleY + u.bodyOffsetY, hz = kHullNodes[n].nz * 17.0f * u.bodyScaleZ + u.bodyOffsetZ;
            float rx = hx * cosY + hz * sinY, rz = -hx * sinY + hz * cosY, ry = hy * cosP - rz * sinP;
            int sx = cx + static_cast<int>(rx * pxScale), sy = baseCy - static_cast<int>(ry * pxScale);
            int occupiedSlot = -1;
            for (int t = 0; t < u.numTurrets; ++t) if (std::hypot(u.turrets[t].snapX - hx, (u.turrets[t].snapZ + u.bodyOffsetZ) - hz) < 6.5f) { occupiedSlot = t; break; }
            PIELIGHT nodeCol = (occupiedSlot >= 0) ? pal_RGBA(75, 255, 145, 255) : pal_RGBA(255, 215, 70, 240);
            PIELIGHT fillCol = (occupiedSlot >= 0) ? pal_RGBA(20, 120, 65, 220) : pal_RGBA(40, 35, 12, 200);
            pie_UniTransBoxFill(sx - 8, sy - 8, sx + 8, sy + 8, fillCol); iV_Box(sx - 8, sy - 8, sx + 8, sy + 8, nodeCol);
            iV_Line(sx - 11, sy, sx + 11, sy, nodeCol); iV_Line(sx, sy - 11, sx, sy + 11, nodeCol);
            clickBoxes.push_back({ 500 + n, sx - 11, sy - 11, 22, 22 });
            if (occupiedSlot >= 0) { std::string tag = "M" + std::to_string(occupiedSlot + 1); iV_SetTextColour(pal_RGBA(120, 255, 175, 255)); iV_DrawText(tag.c_str(), sx + 10, sy + 4, font_regular); }
        }
        std::string diag = "bIdx=" + std::to_string(bIdx) + " pIdx=" + std::to_string(pIdx) +
            " bodyOK=" + std::to_string(bIdx >= 0 && asBodyStats[bIdx].pIMD != nullptr) +
            " tex=" + std::to_string((bIdx >= 0 && asBodyStats[bIdx].pIMD && asBodyStats[bIdx].pIMD->displayModel()) ? (int)asBodyStats[bIdx].pIMD->displayModel()->getTextures().texpage : -1);
        iV_SetTextColour(pal_RGBA(175, 220, 255, 240)); iV_DrawText("HOLD RIGHT-CLICK & DRAG TO ORBIT 3D MODEL | CLICK YELLOW NODES TO SNAP TURRETS", cx - boxW / 2 + 12, cy - boxH / 2 + 18, font_regular);
    }
}

void usaOpenUnitEditor()
{
    usaEnsureOverworldDesignAssets(); usaLoadSuperHeavyBlueprints();
    selectedBpIdx = std::max(0, std::min(static_cast<int>(blueprints.size()) - 1, selectedBpIdx));
    activeTurretSlot = 0; renamingClass = false; editorOpen = true;
}
void usaCloseUnitEditor() { renamingClass = false; editorOpen = false; }
bool usaIsUnitEditorOpen() { return editorOpen; }
void usaUpdateAndDrawUnitEditor()
{
    if (!editorOpen) return;
    initDefaultBlueprints(); clickBoxes.clear();
    int sw = pie_GetVideoBufferWidth(), sh = pie_GetVideoBufferHeight();
    pie_UniTransBoxFill(0, 0, sw, 116, pal_RGBA(6, 10, 18, 250)); pie_UniTransBoxFill(0, sh - 28, sw, sh, pal_RGBA(6, 10, 18, 250)); pie_UniTransBoxFill(0, 116, 338, sh - 28, pal_RGBA(6, 10, 18, 250)); pie_UniTransBoxFill(sw - 338, 116, sw, sh - 28, pal_RGBA(6, 10, 18, 250));
    iV_Box(12, 12, sw - 12, sh - 12, pal_RGBA(65, 165, 255, 255));
    SuperHeavyBlueprint& u = blueprints[selectedBpIdx];
    if (renamingClass)
    {
        for (int k = 32; k <= 126; ++k) if (keyPressed(static_cast<KEY_CODE>(k)) && u.name.size() < 24) { char ch = static_cast<char>(k); if (!keyDown(KEY_LSHIFT) && !keyDown(KEY_RSHIFT) && ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch + 32); u.name.push_back(ch); }
        if (keyPressed(KEY_BACKSPACE) && !u.name.empty()) u.name.pop_back();
        if (keyPressed(KEY_RETURN)) { renamingClass = false; statusBanner = "RENAMED CLASS TO: " + u.name; }
    }
    iV_SetTextColour(pal_RGBA(110, 210, 255, 255)); iV_DrawText("UNIT CLASS EDITOR - HULL SCALING & SURFACE HARDPOINTS", 28, 42, font_large);
    iV_SetTextColour(pal_RGBA(200, 225, 245, 255)); iV_DrawText(statusBanner.c_str(), 28, 64, font_regular);

    drawBtn(101, 28, 76, 110, 28, "< PREV CLASS", pal_RGBA(22, 44, 72, 240), pal_RGBA(80, 170, 255, 255));
    drawBtn(102, 146, 76, 230, 28, renamingClass ? ("TYPE: " + u.name + "_") : ("CLASS: " + u.name + " [EDIT]"), pal_RGBA(30, 62, 98, 250), pal_RGBA(120, 210, 255, 255));
    drawBtn(103, 384, 76, 110, 28, "NEXT CLASS >", pal_RGBA(22, 44, 72, 240), pal_RGBA(80, 170, 255, 255));
    drawBtn(104, 504, 76, 130, 28, "+ NEW CLASS", pal_RGBA(20, 75, 48, 245), pal_RGBA(90, 235, 150, 255));
    drawBtn(107, sw - 456, 76, 168, 28, "LAUNCH SIMULATOR", pal_RGBA(120, 68, 16, 250), pal_RGBA(255, 195, 70, 255));
    drawBtn(105, sw - 280, 76, 130, 28, "SAVE ALL", pal_RGBA(24, 82, 52, 245), pal_RGBA(90, 240, 155, 255));
    drawBtn(106, sw - 142, 76, 114, 28, "EXIT", pal_RGBA(110, 28, 28, 245), pal_RGBA(255, 100, 100, 255));

    int lx = 28, lw = 310;
    pie_UniTransBoxFill(lx, 116, lx + lw, sh - 28, pal_RGBA(12, 20, 34, 240)); iV_Box(lx, 116, lx + lw, sh - 28, pal_RGBA(60, 140, 210, 220));
    iV_DrawText("1. SUPER-HEAVY CHASSIS (L/R CLICK)", lx + 12, 138, font_regular);
    drawBtn(201, lx + 12, 148, lw - 24, 26, "HULL: " + u.bodyId, pal_RGBA(26, 52, 82, 240), pal_RGBA(90, 180, 255, 255));
    iV_DrawText(("UNIFORM SCALE: " + std::to_string(u.bodyScaleX).substr(0, 4) + "x").c_str(), lx + 12, 194, font_regular);
    drawBtn(202, lx + 180, 178, 52, 24, "-0.15", pal_RGBA(40, 45, 65, 240), pal_RGBA(120, 180, 240, 255));
    drawBtn(203, lx + 238, 178, 56, 24, "+0.15", pal_RGBA(40, 45, 65, 240), pal_RGBA(120, 180, 240, 255));
    iV_DrawText(("X:" + std::to_string(u.bodyScaleX).substr(0, 4) + " Y:" + std::to_string(u.bodyScaleY).substr(0, 4) + " Z:" + std::to_string(u.bodyScaleZ).substr(0, 4)).c_str(), lx + 12, 220, font_regular);
    drawBtn(204, lx + 12, 228, 90, 24, "WID +0.1", pal_RGBA(28, 48, 72, 240), pal_RGBA(90, 170, 240, 255));
    drawBtn(205, lx + 108, 228, 90, 24, "HGT +0.1", pal_RGBA(28, 48, 72, 240), pal_RGBA(90, 170, 240, 255));
    drawBtn(206, lx + 204, 228, 90, 24, "LEN +0.1", pal_RGBA(28, 48, 72, 240), pal_RGBA(90, 170, 240, 255));
    iV_DrawText(("HULL LIFT Y:" + std::to_string((int)u.bodyOffsetY) + " FORE Z:" + std::to_string((int)u.bodyOffsetZ)).c_str(), lx + 12, 272, font_regular);
    drawBtn(207, lx + 12, 280, 66, 24, "LIFT -2", pal_RGBA(35, 55, 80, 240), pal_RGBA(100, 190, 255, 255));
    drawBtn(208, lx + 84, 280, 66, 24, "LIFT +2", pal_RGBA(35, 55, 80, 240), pal_RGBA(100, 190, 255, 255));
    drawBtn(209, lx + 158, 280, 66, 24, "FORE -2", pal_RGBA(35, 55, 80, 240), pal_RGBA(100, 190, 255, 255));
    drawBtn(210, lx + 230, 280, 66, 24, "FORE +2", pal_RGBA(35, 55, 80, 240), pal_RGBA(100, 190, 255, 255));
    iV_DrawText("2. MATCHED PROPULSION SYSTEM", lx + 12, 330, font_regular);
    drawBtn(220, lx + 12, 340, lw - 24, 26, "DRIVE: " + u.propId, pal_RGBA(26, 52, 82, 240), pal_RGBA(90, 180, 255, 255));
    drawBtn(221, lx + 12, 374, lw - 24, 26, u.matchChassisScale ? "MATCH CHASSIS SCALE: ON [AUTO]" : "MATCH CHASSIS SCALE: OFF [SYNC NOW]", pal_RGBA(22, 84, 62, 245), pal_RGBA(95, 240, 175, 255));
    iV_DrawText(("DRIVE X:" + std::to_string(u.propScaleX).substr(0, 4) + " Y:" + std::to_string(u.propScaleY).substr(0, 4) + " Z:" + std::to_string(u.propScaleZ).substr(0, 4)).c_str(), lx + 12, 420, font_regular);
    drawBtn(222, lx + 12, 428, 90, 24, "DRV -0.1", pal_RGBA(35, 55, 80, 240), pal_RGBA(100, 190, 255, 255));
    drawBtn(223, lx + 108, 428, 90, 24, "DRV +0.1", pal_RGBA(35, 55, 80, 240), pal_RGBA(100, 190, 255, 255));
    drawBtn(224, lx + 204, 428, 90, 24, "SNAP Y-2", pal_RGBA(35, 55, 80, 240), pal_RGBA(100, 190, 255, 255));
    int estHp = static_cast<int>(450.0f * u.bodyScaleX * u.bodyScaleY * u.bodyScaleZ);
    iV_SetTextColour(pal_RGBA(120, 245, 165, 255)); iV_DrawText(("CLASS HP: " + std::to_string(estHp) + "   TURRETS: " + std::to_string(u.numTurrets)).c_str(), lx + 12, sh - 84, font_regular);
    drawBtn(230, lx + 12, sh - 68, lw - 24, 28, "ADD TO CAMPAIGN SQUAD", pal_RGBA(24, 92, 46, 250), pal_RGBA(100, 250, 155, 255));

    int rx = sw - 338, viewW = std::max(280, rx - (lx + lw) - 24), viewH = sh - 188, viewCx = lx + lw + 12 + viewW / 2, viewCy = 116 + viewH / 2;
    draw3DPreview(u, viewCx, viewCy, viewW, viewH);

    int camY = sh - 60;
    drawBtn(301, viewCx - 210, camY, 80, 26, "ROT LEFT", pal_RGBA(25, 45, 70, 240), pal_RGBA(90, 175, 250, 255));
    drawBtn(302, viewCx - 122, camY, 86, 26, autoSpin ? "SPIN: ON" : "SPIN: OFF", pal_RGBA(25, 65, 85, 240), pal_RGBA(90, 210, 255, 255));
    drawBtn(303, viewCx - 28, camY, 80, 26, "ROT RIGHT", pal_RGBA(25, 45, 70, 240), pal_RGBA(90, 175, 250, 255));
    drawBtn(304, viewCx + 60, camY, 70, 26, "ZOOM -", pal_RGBA(25, 45, 70, 240), pal_RGBA(90, 175, 250, 255));
    drawBtn(305, viewCx + 138, camY, 70, 26, "ZOOM +", pal_RGBA(25, 45, 70, 240), pal_RGBA(90, 175, 250, 255));

    pie_UniTransBoxFill(rx, 116, rx + lw, sh - 28, pal_RGBA(12, 20, 34, 240)); iV_Box(rx, 116, rx + lw, sh - 28, pal_RGBA(60, 140, 210, 220));
    iV_SetTextColour(WZCOL_WHITE); iV_DrawText("3. TURRET HARDPOINT SNAPPING", rx + 12, 138, font_regular);
    int maxAllowed = (std::max({ u.bodyScaleX, u.bodyScaleY, u.bodyScaleZ }) >= 1.60f) ? 4 : 3; iV_DrawText(("TURRETS: " + std::to_string(u.numTurrets) + " / " + std::to_string(maxAllowed) + (maxAllowed == 3 ? " (1.65x=4)" : " (MAX)")).c_str(), rx + 12, 162, font_regular);
    drawBtn(401, rx + 180, 146, 52, 24, "-1", pal_RGBA(45, 45, 65, 240), pal_RGBA(120, 180, 240, 255));
    drawBtn(402, rx + 238, 146, 56, 24, "+1", pal_RGBA(45, 45, 65, 240), pal_RGBA(120, 180, 240, 255));
    for (int t = 0; t < 4; ++t)
        drawBtn(410 + t, rx + 12 + t * 98, 178, 92, 24, "MOUNT " + std::to_string(t + 1), (t == activeTurretSlot) ? pal_RGBA(35, 95, 145, 255) : pal_RGBA(22, 36, 58, 240), (t == activeTurretSlot) ? pal_RGBA(130, 230, 255, 255) : pal_RGBA(70, 130, 190, 220));
    auto& tr = u.turrets[activeTurretSlot];
    drawBtn(420, rx + 12, 212, lw - 24, 26, "WEAPON: " + tr.weaponId, pal_RGBA(26, 52, 82, 240), pal_RGBA(90, 180, 255, 255));
    drawBtn(421, rx + 12, 246, lw - 24, 26, std::string("PRESET: ") + kPresetNames[tr.presetIndex % 6], pal_RGBA(24, 76, 92, 245), pal_RGBA(100, 225, 255, 255));
    iV_DrawText(("TURRET SCALE: " + std::to_string(tr.scale).substr(0, 4) + "x").c_str(), rx + 12, 294, font_regular);
    drawBtn(422, rx + 180, 278, 52, 24, "-0.15", pal_RGBA(40, 45, 65, 240), pal_RGBA(120, 180, 240, 255));
    drawBtn(423, rx + 238, 278, 56, 24, "+0.15", pal_RGBA(40, 45, 65, 240), pal_RGBA(120, 180, 240, 255));
    iV_DrawText(("SNAP X: " + std::to_string((int)tr.snapX)).c_str(), rx + 12, 326, font_regular);
    drawBtn(430, rx + 180, 310, 52, 24, "X -3", pal_RGBA(35, 55, 80, 240), pal_RGBA(100, 190, 255, 255));
    drawBtn(431, rx + 238, 310, 56, 24, "X +3", pal_RGBA(35, 55, 80, 240), pal_RGBA(100, 190, 255, 255));
    iV_DrawText(("SNAP Y: " + std::to_string((int)tr.snapY)).c_str(), rx + 12, 358, font_regular);
    drawBtn(432, rx + 180, 342, 52, 24, "Y -3", pal_RGBA(35, 55, 80, 240), pal_RGBA(100, 190, 255, 255));
    drawBtn(433, rx + 238, 342, 56, 24, "Y +3", pal_RGBA(35, 55, 80, 240), pal_RGBA(100, 190, 255, 255));
    iV_DrawText(("SNAP Z: " + std::to_string((int)tr.snapZ)).c_str(), rx + 12, 390, font_regular);
    drawBtn(434, rx + 180, 374, 52, 24, "Z -3", pal_RGBA(35, 55, 80, 240), pal_RGBA(100, 190, 255, 255));
    drawBtn(435, rx + 238, 374, 56, 24, "Z +3", pal_RGBA(35, 55, 80, 240), pal_RGBA(100, 190, 255, 255));
}
bool usaHandleUnitEditorClick(int mx, int my, bool rightClick)
{
    if (!editorOpen) return false;
    initDefaultBlueprints();
    SuperHeavyBlueprint& u = blueprints[selectedBpIdx];
    auto& tr = u.turrets[activeTurretSlot];
    auto syncScaleRules = [&]() { int maxT = (std::max({ u.bodyScaleX, u.bodyScaleY, u.bodyScaleZ }) >= 1.60f) ? 4 : 3; if (maxT == 4 && u.numTurrets == 3) u.numTurrets = 4; if (u.numTurrets > maxT) u.numTurrets = maxT; activeTurretSlot = std::min(activeTurretSlot, u.numTurrets - 1); if (u.matchChassisScale) { u.propScaleX = u.bodyScaleX; u.propScaleY = std::max(1.0f, u.bodyScaleY * 0.94f); u.propScaleZ = u.bodyScaleZ; u.bodyOffsetY = (u.propScaleY - 1.0f) * 12.0f; u.propOffsetY = 0.0f; u.propOffsetZ = 0.0f; } };
    for (const auto& b : clickBoxes)
    {
        if (mx < b.x || mx > b.x + b.w || my < b.y || my > b.y + b.h) continue;
        switch (b.id)
        {
        case 102: renamingClass = !renamingClass; statusBanner = renamingClass ? "TYPE CLASS NAME ON KEYBOARD (ENTER TO CONFIRM)" : ("CONFIRMED: " + u.name); break;
        case 101: renamingClass = false; selectedBpIdx = (selectedBpIdx + static_cast<int>(blueprints.size()) - 1) % static_cast<int>(blueprints.size()); activeTurretSlot = 0; break;
        case 103: renamingClass = false; selectedBpIdx = (selectedBpIdx + 1) % static_cast<int>(blueprints.size()); activeTurretSlot = 0; break;
        case 104: { SuperHeavyBlueprint nb = u; nb.name = "Colossus Mk " + std::to_string(blueprints.size() + 1); blueprints.push_back(nb); selectedBpIdx = static_cast<int>(blueprints.size()) - 1; statusBanner = "CREATED: " + nb.name; break; }
        case 105: usaSaveSuperHeavyBlueprints(); break;
        case 107: usaSimulatorClassName = u.name; usaSaveSuperHeavyBlueprints(); usaCloseUnitEditor(); usaLaunchSimulatorArena(); return true;
        case 106: usaSaveSuperHeavyBlueprints(); usaCloseUnitEditor(); break;
        case 201: { int idx = 0; for (int i = 0; i < 8; ++i) if (u.bodyId == kBodies[i]) { idx = rightClick ? ((i + 7) % 8) : ((i + 1) % 8); break; } u.bodyId = kBodies[idx]; statusBanner = std::string("HULL: ") + kBodyNames[idx]; break; }
        case 202: u.bodyScaleX = std::max(1.25f, u.bodyScaleX - 0.15f); u.bodyScaleY = std::max(1.25f, u.bodyScaleY - 0.15f); u.bodyScaleZ = std::max(1.25f, u.bodyScaleZ - 0.15f); syncScaleRules(); break;
        case 203: u.bodyScaleX = std::min(3.0f, u.bodyScaleX + 0.15f); u.bodyScaleY = std::min(3.0f, u.bodyScaleY + 0.15f); u.bodyScaleZ = std::min(3.0f, u.bodyScaleZ + 0.15f); syncScaleRules(); break;
        case 204: u.bodyScaleX = (u.bodyScaleX >= 2.8f) ? 1.35f : (u.bodyScaleX + 0.15f); syncScaleRules(); break;
        case 205: u.bodyScaleY = (u.bodyScaleY >= 2.8f) ? 1.35f : (u.bodyScaleY + 0.15f); syncScaleRules(); break;
        case 206: u.bodyScaleZ = (u.bodyScaleZ >= 2.8f) ? 1.35f : (u.bodyScaleZ + 0.15f); syncScaleRules(); break;
        case 207: u.bodyOffsetY -= 2.0f; break;
        case 208: u.bodyOffsetY += 2.0f; break;
        case 209: u.bodyOffsetZ -= 2.0f; break;
        case 210: u.bodyOffsetZ += 2.0f; break;
        case 220: { int idx = 0; for (int i = 0; i < 5; ++i) if (u.propId == kProps[i]) { idx = rightClick ? ((i + 4) % 5) : ((i + 1) % 5); break; } u.propId = kProps[idx]; statusBanner = std::string("DRIVE: ") + kPropNames[idx]; break; }
        case 221: u.matchChassisScale = !u.matchChassisScale; if (u.matchChassisScale) syncScaleRules(); statusBanner = u.matchChassisScale ? "PROPULSION LOCKED TO CHASSIS SCALE" : "PROPULSION SCALE UNLOCKED (MANUAL)"; break;
        case 222: u.propScaleX = std::max(1.0f, u.propScaleX - 0.1f); u.propScaleZ = std::max(1.0f, u.propScaleZ - 0.1f); break;
        case 223: u.propScaleX = std::min(3.0f, u.propScaleX + 0.1f); u.propScaleZ = std::min(3.0f, u.propScaleZ + 0.1f); break;
        case 224: u.propOffsetY -= 2.0f; break;
        case 230: { usaSaveSuperHeavyBlueprints(); UsaCampaignState& cs = getUsaCampaignState(); cs.squad.push_back({ u.name, u.bodyId, u.propId, u.turrets[0].weaponId, 0, 1 }); cs.invasionLoadout.push_back({ u.name, u.bodyId, u.propId, u.turrets[0].weaponId, 0, 1 }); statusBanner = "ADDED " + u.name + " TO CAMPAIGN SQUAD!"; break; }
        case 301: autoSpin = false; previewYaw -= 18.0f; break;
        case 302: autoSpin = !autoSpin; break;
        case 303: autoSpin = false; previewYaw += 18.0f; break;
        case 304: previewZoom = std::max(0.55f, previewZoom - 0.15f); break;
        case 305: previewZoom = std::min(1.85f, previewZoom + 0.15f); break;
        case 401: u.numTurrets = std::max(1, u.numTurrets - 1); activeTurretSlot = std::min(activeTurretSlot, u.numTurrets - 1); break;
        case 402: { int maxT = (std::max({ u.bodyScaleX, u.bodyScaleY, u.bodyScaleZ }) >= 1.60f) ? 4 : 3; u.numTurrets = std::min(maxT, u.numTurrets + 1); break; }
        case 410: case 411: case 412: case 413: activeTurretSlot = b.id - 410; break;
        case 420: { int idx = 0; for (int i = 0; i < 9; ++i) if (tr.weaponId == kWeapons[i]) { idx = rightClick ? ((i + 8) % 9) : ((i + 1) % 9); break; } tr.weaponId = kWeapons[idx]; statusBanner = std::string("WEAPON: ") + kWeapNames[idx]; break; }
        case 421: tr.presetIndex = (tr.presetIndex + 1) % 6; applyTurretPreset(u, activeTurretSlot); statusBanner = std::string("PRESET: ") + kPresetNames[tr.presetIndex]; break;
        case 422: tr.scale = std::max(0.85f, tr.scale - 0.15f); break;
        case 423: tr.scale = std::min(2.50f, tr.scale + 0.15f); break;
        case 430: tr.snapX -= 3.0f; break;
        case 431: tr.snapX += 3.0f; break;
        case 432: tr.snapY -= 3.0f; break;
        case 433: tr.snapY += 3.0f; break;
        case 434: tr.snapZ -= 3.0f; break;
        case 435: tr.snapZ += 3.0f; break;
        case 500: case 501: case 502: case 503: case 504: case 505: case 506: case 507: case 508: { int ni = b.id - 500; if (activeTurretSlot >= u.numTurrets) u.numTurrets = activeTurretSlot + 1; auto& sr = u.turrets[activeTurretSlot]; sr.snapX = kHullNodes[ni].nx * 15.0f * u.bodyScaleX; sr.snapY = kHullNodes[ni].ny * 11.0f * u.bodyScaleY; sr.snapZ = kHullNodes[ni].nz * 17.0f * u.bodyScaleZ; statusBanner = std::string("MOUNT ") + std::to_string(activeTurretSlot + 1) + " SNAPPED TO " + kHullNodes[ni].label; break; }
        }
        return true;
    }
    return true;
}

void usaLaunchSimulatorArena()
{
    bUsaSimulatorMode = true;
    bUsaPrebattle = false;
    bUsaStripVanillaLobby = true;
    bUsaSessionActive = true;
    std::string jStr = "{\n  \"challenge\": {\n    \"name\": \"Unit Class Simulator Arena\",\n    \"map\": \"Sk-Rush\",\n    \"maxPlayers\": 2,\n    \"scavengers\": false,\n    \"allowPositionChange\": false,\n    \"bases\": 0,\n    \"powerLevel\": 2,\n    \"alliances\": 0,\n    \"techLevel\": 3,\n    \"player_0\": { \"name\": \"The Project\", \"team\": 0, \"position\": 0, \"colour\": 0 },\n    \"player_1\": { \"name\": \"Simulator Range\", \"team\": 1, \"position\": 1, \"colour\": 7, \"ai\": \"null\" }\n  }\n}\n";
    PHYSFS_mkdir("tests");
    if (PHYSFS_File* f = PHYSFS_openWrite("tests/usa.json")) { PHYSFS_writeBytes(f, jStr.data(), jStr.size()); PHYSFS_close(f); }
    std::string bStr = "{\n  \"stateName\": \"COMBAT TEST ARENA (SK-RUSH)\",\n  \"factionName\": \"Simulator Range\",\n  \"mapName\": \"Sk-Rush\",\n  \"mode\": \"simulator\",\n  \"turn\": 1,\n  \"power\": 50000\n}\n";
    if (PHYSFS_File* bf = PHYSFS_openWrite("tests/usa_briefing.json")) { PHYSFS_writeBytes(bf, bStr.data(), bStr.size()); PHYSFS_close(bf); }
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