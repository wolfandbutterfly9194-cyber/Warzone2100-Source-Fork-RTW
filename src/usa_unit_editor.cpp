#include <chrono>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
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
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
extern bool bUsaPrebattle, bUsaStripVanillaLobby, bUsaSessionActive; extern std::string challengeName;
extern void usaSetSkirmishTestName(const std::string& name); extern bool usaEnsureOverworldDesignAssets();
bool bUsaSimulatorMode = false; std::string usaSimulatorClassName = "";
namespace
{
    bool editorOpen = false, blueprintsLoadedOnce = false, autoSpin = false, renamingClass = false, isDraggingView = false;
    std::vector<SuperHeavyBlueprint> blueprints; SuperHeavyBlueprint activeDesign;
    std::string statusBanner = "BLANK DESIGN CANVAS - SELECT CHASSIS & PROPULSION TO BEGIN";
    std::vector<SuperHeavyBlueprint> undoStack, redoStack;
    void pushUndoState() { undoStack.push_back(activeDesign); if (undoStack.size() > 40) undoStack.erase(undoStack.begin()); redoStack.clear(); }
    void doUndo() { if (undoStack.empty()) { statusBanner = "NOTHING TO UNDO"; return; } redoStack.push_back(activeDesign); activeDesign = undoStack.back(); undoStack.pop_back(); statusBanner = "UNDID LAST ACTION"; }
    void doRedo() { if (redoStack.empty()) { statusBanner = "NOTHING TO REDO"; return; } undoStack.push_back(activeDesign); activeDesign = redoStack.back(); redoStack.pop_back(); statusBanner = "REDID ACTION"; }
    int activeTurretSlot = 0, pickerMode = 0, lastDragX = 0, lastDragY = 0, deleteConfirmIdx = -1;
    float previewYaw = 35.0f, previewPitch = -22.0f, previewZoom = 1.0f;
    struct ButtonBox { int id, x, y, w, h; }; std::vector<ButtonBox> clickBoxes;
    const char* kBodies[] = { "Body12SUP", "Body14SUP", "Body11ABT", "Body9REC", "Body10MBT", "Body6SUPP", "Body7ABT", "Body5REC", "Body4ABT", "Body1REC" };
    const char* kBodyNames[] = { "Dragon Super", "Wyvern Super", "Python Heavy", "Tiger Super", "Vengeance Super", "Panther Med", "Retribution", "Cobra Med", "Bug Heavy", "Viper Light" };
    const char* kProps[] = { "tracked01", "HalfTrack", "wheeled01", "hover01", "V-Tol" };
    const char* kPropNames[] = { "Heavy Tracks", "Half-Tracks", "Heavy Wheels", "Hover Skirt", "Heavy VTOL" };
    const char* kWeapons[] = { "Cannon375mmMk1", "Cannon2A-TMk1", "Rocket-HvyA-T", "Missile-A-T", "RailGun3Mk1", "Laser2PULSEMk1", "Howitzer105Mk1", "Rocket-Pod", "MG3Mk1" };
    const char* kWeapNames[] = { "Heavy Cannon", "Medium Cannon", "Tank Killer", "Scourge AT", "Gauss Cannon", "Pulse Laser", "Howitzer", "Mini-Rocket", "Heavy MG" };
    const char* kPresetNames[] = { "Roof Center", "Forward Deck", "Rear Deck", "Left Sponson", "Right Sponson", "High Command" };
    void resetBlankDesign() {
        activeDesign = SuperHeavyBlueprint(); activeDesign.name = "New Unit Class"; activeDesign.bodyId = ""; activeDesign.propId = "";
        activeDesign.bodyScaleX = 1.35f; activeDesign.bodyScaleY = 1.35f; activeDesign.bodyScaleZ = 1.35f; activeDesign.bodyOffsetY = 5.0f;
        activeDesign.matchChassisScale = true; activeDesign.propScaleX = 1.35f; activeDesign.propScaleY = 1.28f; activeDesign.propScaleZ = 1.35f;
        activeDesign.numTurrets = 0; for (int t = 0; t < 4; ++t) activeDesign.turrets[t] = { "Cannon375mmMk1", 1.15f, 0.0f, 11.0f, 0.0f, t }; activeTurretSlot = 0;
    }
}
const std::vector<SuperHeavyBlueprint>& usaGetSuperHeavyBlueprints() { if (!blueprintsLoadedOnce) usaLoadSuperHeavyBlueprints(); return blueprints; }
const SuperHeavyBlueprint* usaFindSuperHeavyByName(const std::string& name) {
    if (!blueprintsLoadedOnce) usaLoadSuperHeavyBlueprints();
    if (!activeDesign.bodyId.empty() && !activeDesign.propId.empty() && activeDesign.name == name) return &activeDesign;
    for (const auto& bp : blueprints) if (bp.name == name) return &bp;
    return nullptr;
}
void usaSaveSuperHeavyBlueprints() {
    std::ofstream out("usa_superheavy_units.json", std::ios::binary); if (!out.is_open()) return;
    out << "{\n  \"units\": [\n";
    for (size_t i = 0; i < blueprints.size(); ++i) {
        const auto& u = blueprints[i];
        out << "    {\"name\":\"" << u.name << "\",\"bodyId\":\"" << u.bodyId << "\",\"propId\":\"" << u.propId << "\","
            << "\"bodyScale\":[" << u.bodyScaleX << "," << u.bodyScaleY << "," << u.bodyScaleZ << "],\"bodyOff\":[" << u.bodyOffsetY << "," << u.bodyOffsetZ << "],"
            << "\"propScale\":[" << u.propScaleX << "," << u.propScaleY << "," << u.propScaleZ << "],\"propOff\":[" << u.propOffsetY << "," << u.propOffsetZ << "],"
            << "\"numTurrets\":" << u.numTurrets << ",\"turrets\":[";
        for (int t = 0; t < 4; ++t) out << "{\"w\":\"" << u.turrets[t].weaponId << "\",\"s\":" << u.turrets[t].scale << ",\"x\":" << u.turrets[t].snapX << ",\"y\":" << u.turrets[t].snapY << ",\"z\":" << u.turrets[t].snapZ << ",\"p\":" << u.turrets[t].presetIndex << "}" << (t < 3 ? "," : "");
        out << "]}" << (i + 1 < blueprints.size() ? "," : "") << "\n";
    }
    out << "  ]\n}\n";
}
void usaLoadSuperHeavyBlueprints() {
    blueprintsLoadedOnce = true; std::ifstream in("usa_superheavy_units.json", std::ios::binary); if (!in.is_open()) return;
    std::stringstream ss; ss << in.rdbuf(); std::string raw = ss.str(); std::vector<SuperHeavyBlueprint> loaded; size_t pos = 0;
    while ((pos = raw.find("\"name\":", pos)) != std::string::npos) {
        SuperHeavyBlueprint u;
        auto readStr = [&](const std::string& key, size_t from) -> std::string { size_t k = raw.find(key, from); if (k == std::string::npos) return ""; size_t q1 = raw.find('"', k + key.size()), q2 = (q1 != std::string::npos) ? raw.find('"', q1 + 1) : std::string::npos; return (q1 != std::string::npos && q2 != std::string::npos) ? raw.substr(q1 + 1, q2 - q1 - 1) : ""; };
        u.name = readStr("\"name\":", pos); u.bodyId = readStr("\"bodyId\":", pos); u.propId = readStr("\"propId\":", pos);
        size_t bs = raw.find("\"bodyScale\":", pos); if (bs != std::string::npos) std::sscanf(raw.c_str() + bs + 12, "[%f,%f,%f]", &u.bodyScaleX, &u.bodyScaleY, &u.bodyScaleZ);
        size_t bo = raw.find("\"bodyOff\":", pos); if (bo != std::string::npos) std::sscanf(raw.c_str() + bo + 10, "[%f,%f]", &u.bodyOffsetY, &u.bodyOffsetZ);
        size_t ps = raw.find("\"propScale\":", pos); if (ps != std::string::npos) std::sscanf(raw.c_str() + ps + 12, "[%f,%f,%f]", &u.propScaleX, &u.propScaleY, &u.propScaleZ);
        size_t po = raw.find("\"propOff\":", pos); if (po != std::string::npos) std::sscanf(raw.c_str() + po + 10, "[%f,%f]", &u.propOffsetY, &u.propOffsetZ);
        size_t nt = raw.find("\"numTurrets\":", pos); if (nt != std::string::npos) std::sscanf(raw.c_str() + nt + 13, "%d", &u.numTurrets);
        size_t tp = raw.find("\"turrets\":", pos);
        for (int t = 0; t < 4 && tp != std::string::npos; ++t) { size_t ob = raw.find('{', tp); if (ob == std::string::npos) break; u.turrets[t].weaponId = readStr("\"w\":", ob); size_t sp = raw.find("\"s\":", ob); if (sp != std::string::npos) std::sscanf(raw.c_str() + sp + 4, "%f,\"x\":%f,\"y\":%f,\"z\":%f,\"p\":%d", &u.turrets[t].scale, &u.turrets[t].snapX, &u.turrets[t].snapY, &u.turrets[t].snapZ, &u.turrets[t].presetIndex); tp = ob + 1; }
        if (!u.name.empty() && !u.bodyId.empty() && !u.propId.empty()) loaded.push_back(u); pos += 8;
    }
    blueprints = std::move(loaded);
}
void usaRegisterSuperHeavyTemplatesForPlayer(int player) {
    if (!blueprintsLoadedOnce) usaLoadSuperHeavyBlueprints();
    auto regOne = [&](const SuperHeavyBlueprint& u) {
        if (u.bodyId.empty() || u.propId.empty()) return;
        int bIdx = getCompFromID(COMP_BODY, WzString::fromUtf8(u.bodyId)), pIdx = getCompFromID(COMP_PROPULSION, WzString::fromUtf8(u.propId)); if (bIdx < 0 || pIdx < 0) return;
        auto tmpl = std::make_unique<DROID_TEMPLATE>(); tmpl->name = WzString::fromUtf8(u.name); tmpl->id = WzString::fromUtf8(u.name);
        for (int k = 0; k < DROID_MAXCOMP; ++k) tmpl->asParts[k] = 0; tmpl->asParts[COMP_BODY] = bIdx; tmpl->asParts[COMP_PROPULSION] = pIdx;
        int wc = 0; for (int t = 0; t < std::min(4, u.numTurrets); ++t) { int wi = getCompFromID(COMP_WEAPON, WzString::fromUtf8(u.turrets[t].weaponId)); if (wi >= 0) { *(tmpl->asWeaps + wc) = wi; wc++; } }
        tmpl->numWeaps = wc; tmpl->droidType = DROID_WEAPON; addTemplate(player, std::move(tmpl));
    };
    if (!activeDesign.bodyId.empty() && !activeDesign.propId.empty()) regOne(activeDesign);
    for (const auto& u : blueprints) regOne(u);
}
namespace
{
    void applyTurretPreset(SuperHeavyBlueprint& u, int slot) {
        auto& tr = u.turrets[slot]; float hx = 14.0f * u.bodyScaleX, hy = 10.0f * u.bodyScaleY, hz = 16.0f * u.bodyScaleZ;
        switch (tr.presetIndex % 6) {
        case 0: tr.snapX = 0.0f; tr.snapY = hy; tr.snapZ = 0.0f; break; case 1: tr.snapX = 0.0f; tr.snapY = hy * 0.8f; tr.snapZ = hz; break;
        case 2: tr.snapX = 0.0f; tr.snapY = hy * 1.1f; tr.snapZ = -hz; break; case 3: tr.snapX = -hx; tr.snapY = hy * 0.5f; tr.snapZ = 0.0f; break;
        case 4: tr.snapX = hx; tr.snapY = hy * 0.5f; tr.snapZ = 0.0f; break; case 5: tr.snapX = 0.0f; tr.snapY = hy * 1.5f; tr.snapZ = -4.0f; break;
        }
    }
    void drawBtn(int id, int x, int y, int w, int h, const std::string& label, PIELIGHT fill, PIELIGHT border) {
        clickBoxes.push_back({ id, x, y, w, h }); pie_UniTransBoxFill(x, y, x + w, y + h, fill); iV_Box(x, y, x + w, y + h, border);
        iV_SetTextColour(WZCOL_WHITE); int tw = iV_GetTextWidth(label.c_str(), font_regular); iV_DrawText(label.c_str(), x + std::max(6, (w - tw) / 2), y + (h / 2) + 4, font_regular);
    }
    struct HullNodeSpec { const char* label; float nx, ny, nz; };
    const HullNodeSpec kHullNodes[] = { { "ROOF CENTER", 0.0f, 1.05f, 0.0f }, { "FORWARD DECK", 0.0f, 0.82f, 1.05f }, { "REAR DECK", 0.0f, 1.12f, -1.05f }, { "PORT SPONSON", -1.05f, 0.55f, 0.05f }, { "STBD SPONSON", 1.05f, 0.55f, 0.05f }, { "PORT CHEEK", -0.68f, 0.72f, 0.85f }, { "STBD CHEEK", 0.68f, 0.72f, 0.85f }, { "AFT PORT", -0.75f, 0.85f, -0.82f }, { "AFT STBD", 0.75f, 0.85f, -0.82f } };
    void draw3DPreview(const SuperHeavyBlueprint& u, int cx, int cy, int boxW, int boxH) {
        if (pickerMode == 0 && mouseDown(MOUSE_RMB)) {
            int mx = mouseX(), my = mouseY();
            if (!isDraggingView && mx >= cx - boxW / 2 && mx <= cx + boxW / 2 && my >= cy - boxH / 2 && my <= cy + boxH / 2) { isDraggingView = true; lastDragX = mx; lastDragY = my; }
            if (isDraggingView) { autoSpin = false; previewYaw = std::fmod(previewYaw + (mx - lastDragX) * 0.65f + 360.0f, 360.0f); previewPitch = std::max(-75.0f, std::min(15.0f, previewPitch + (my - lastDragY) * 0.55f)); lastDragX = mx; lastDragY = my; }
        } else { isDraggingView = false; }
        pie_UniTransBoxFill(cx - boxW / 2, cy - boxH / 2, cx + boxW / 2, cy + boxH / 2, pal_RGBA(8, 14, 24, 248));
        for (int gx = cx - boxW / 2 + 36; gx < cx + boxW / 2; gx += 36) iV_Line(gx, cy - boxH / 2 + 28, gx, cy + boxH / 2 - 44, pal_RGBA(22, 46, 74, 110));
        for (int gy = cy - boxH / 2 + 36; gy < cy + boxH / 2 - 44; gy += 36) iV_Line(cx - boxW / 2 + 12, gy, cx + boxW / 2 - 12, gy, pal_RGBA(22, 46, 74, 110));
        iV_Box(cx - boxW / 2, cy - boxH / 2, cx + boxW / 2, cy + boxH / 2, pal_RGBA(70, 175, 255, 255));
        if (autoSpin) previewYaw = std::fmod(previewYaw + 0.55f, 360.0f);
        for (int g = -4; g <= 4; ++g) iV_Line(cx + g * 28, cy + boxH / 2 - 42, cx + g * 36, cy + boxH / 2 - 12, pal_RGBA(35, 95, 140, 160));
        if (u.bodyId.empty() && u.propId.empty()) { iV_SetTextColour(pal_RGBA(150, 205, 255, 240)); iV_DrawText("EMPTY WORKSPACE - CLICK [SELECT CHASSIS] & [SELECT PROPULSION] ON LEFT", cx - 220, cy, font_regular); return; }
        int baseCy = cy + 24;
        int bIdx = u.bodyId.empty() ? -1 : getCompFromID(COMP_BODY, WzString::fromUtf8(u.bodyId));
        int pIdx = u.propId.empty() ? -1 : getCompFromID(COMP_PROPULSION, WzString::fromUtf8(u.propId));
        pie_Begin3DScene(); pie_SetGeometricOffset(cx, baseCy);
        float sc = (DROID_BUT_SCALE * previewZoom * 1.65f) / 100.0f;
        glm::mat4 rootMat = glm::translate(glm::vec3(0.f, 0.f, 680.f)) * glm::rotate(previewPitch * 3.14159265f / 180.f, glm::vec3(1.f, 0.f, 0.f)) * glm::rotate(previewYaw * 3.14159265f / 180.f, glm::vec3(0.f, 1.f, 0.f)) * glm::scale(glm::vec3(sc));
        auto drawPart3D = [&](const iIMDBaseShape* baseSh, const glm::mat4& mat) { if (!baseSh) return; for (const iIMDShape* imd = baseSh->displayModel(); imd != nullptr; imd = imd->next.get()) pie_Draw3DButton(imd, imd->getTeamColourForModel(getPlayerColour(0)), mat, glm::mat4(1.f)); };
        if (pIdx >= 0) { glm::mat4 pMat = rootMat * glm::translate(glm::vec3(0.f, u.propOffsetY, u.propOffsetZ)) * glm::scale(glm::vec3(u.propScaleX, u.propScaleY, u.propScaleZ)); if (bIdx >= 0 && !asBodyStats[bIdx].ppIMDList.empty()) { drawPart3D(asBodyStats[bIdx].ppIMDList[pIdx * NUM_PROP_SIDES + LEFT_PROP], pMat); drawPart3D(asBodyStats[bIdx].ppIMDList[pIdx * NUM_PROP_SIDES + RIGHT_PROP], pMat); } else { drawPart3D(asPropulsionStats[pIdx].pIMD, pMat); } }
        if (bIdx >= 0) drawPart3D(asBodyStats[bIdx].pIMD, rootMat * glm::translate(glm::vec3(0.f, u.bodyOffsetY, u.bodyOffsetZ)) * glm::scale(glm::vec3(u.bodyScaleX, u.bodyScaleY, u.bodyScaleZ)));
        for (int t = 0; t < std::min(4, u.numTurrets); ++t) { int wIdx = getCompFromID(COMP_WEAPON, WzString::fromUtf8(u.turrets[t].weaponId)); if (wIdx >= 0) { glm::mat4 tMat = rootMat * glm::translate(glm::vec3(u.turrets[t].snapX, u.bodyOffsetY + u.turrets[t].snapY, u.bodyOffsetZ + u.turrets[t].snapZ)) * glm::scale(glm::vec3(u.turrets[t].scale)); drawPart3D(asWeaponStats[wIdx].pMountGraphic, tMat); drawPart3D(asWeaponStats[wIdx].pIMD, tMat); } }
        pie_BeginInterface(); pie_SetGeometricOffset(pie_GetVideoBufferWidth() / 2, pie_GetVideoBufferHeight() / 2);
        if (bIdx >= 0 && pickerMode == 0) {
            float yawRad = previewYaw * 3.14159265f / 180.0f, pitchRad = -previewPitch * 3.14159265f / 180.0f, cosY = std::cos(yawRad), sinY = std::sin(yawRad), cosP = std::cos(pitchRad), sinP = std::sin(pitchRad), pxScale = 2.45f * previewZoom;
            for (int n = 0; n < 9; ++n) {
                float hx = kHullNodes[n].nx * 15.0f * u.bodyScaleX, hy = kHullNodes[n].ny * 11.0f * u.bodyScaleY + u.bodyOffsetY, hz = kHullNodes[n].nz * 17.0f * u.bodyScaleZ + u.bodyOffsetZ;
                float rx = hx * cosY + hz * sinY, rz = -hx * sinY + hz * cosY, ry = hy * cosP - rz * sinP;
                int sx = cx + static_cast<int>(rx * pxScale), sy = baseCy - static_cast<int>(ry * pxScale), occ = -1;
                for (int t = 0; t < u.numTurrets; ++t) if (std::hypot(u.turrets[t].snapX - hx, (u.turrets[t].snapZ + u.bodyOffsetZ) - hz) < 6.5f) { occ = t; break; }
                PIELIGHT nc = (occ >= 0) ? pal_RGBA(75, 255, 145, 255) : pal_RGBA(255, 215, 70, 240), fc = (occ >= 0) ? pal_RGBA(20, 120, 65, 220) : pal_RGBA(40, 35, 12, 200);
                pie_UniTransBoxFill(sx - 8, sy - 8, sx + 8, sy + 8, fc); iV_Box(sx - 8, sy - 8, sx + 8, sy + 8, nc); iV_Line(sx - 11, sy, sx + 11, sy, nc); iV_Line(sx, sy - 11, sx, sy + 11, nc);
                clickBoxes.push_back({ 500 + n, sx - 11, sy - 11, 22, 22 });
                if (occ >= 0) { std::string tag = "M" + std::to_string(occ + 1); iV_SetTextColour(pal_RGBA(120, 255, 175, 255)); iV_DrawText(tag.c_str(), sx + 10, sy + 4, font_regular); }
            }
        }
        iV_SetTextColour(pal_RGBA(175, 220, 255, 240)); iV_DrawText("HOLD RIGHT-CLICK & DRAG TO ORBIT 3D MODEL | CLICK YELLOW NODES TO ADD/SNAP TURRETS", cx - boxW / 2 + 12, cy - boxH / 2 + 18, font_regular);
    }
}
void usaOpenUnitEditor() { usaEnsureOverworldDesignAssets(); usaLoadSuperHeavyBlueprints(); undoStack.clear(); redoStack.clear(); resetBlankDesign(); pickerMode = 0; renamingClass = false; editorOpen = true; }
void usaCloseUnitEditor() { renamingClass = false; pickerMode = 0; editorOpen = false; }
bool usaIsUnitEditorOpen() { return editorOpen; }
void usaUpdateAndDrawUnitEditor()
{
    if (!editorOpen) return;
    clickBoxes.clear(); int sw = pie_GetVideoBufferWidth(), sh = pie_GetVideoBufferHeight();
    pie_UniTransBoxFill(0, 0, sw, 116, pal_RGBA(6, 10, 18, 250)); pie_UniTransBoxFill(0, sh - 28, sw, sh, pal_RGBA(6, 10, 18, 250));
    pie_UniTransBoxFill(0, 116, 338, sh - 28, pal_RGBA(6, 10, 18, 250)); pie_UniTransBoxFill(sw - 338, 116, sw, sh - 28, pal_RGBA(6, 10, 18, 250));
    iV_Box(12, 12, sw - 12, sh - 12, pal_RGBA(65, 165, 255, 255)); SuperHeavyBlueprint& u = activeDesign;
    if (!renamingClass && (keyDown(KEY_LCTRL) || keyDown(KEY_RCTRL))) { if (keyPressed(KEY_Z)) doUndo(); else if (keyPressed(KEY_Y)) doRedo(); }
    if (renamingClass)
    {
        bool shiftHeld = keyDown(KEY_LSHIFT) || keyDown(KEY_RSHIFT);
#ifdef _WIN32
        if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) || (GetKeyState(VK_CAPITAL) & 0x0001)) shiftHeld = true;
#endif
        for (int k = 32; k <= 126; ++k)
        {
            if (keyPressed(static_cast<KEY_CODE>(k)) && u.name.size() < 24)
            {
                char ch = static_cast<char>(k);
                if (ch >= 'a' && ch <= 'z') ch = shiftHeld ? static_cast<char>(ch - 32) : ch;
                else if (ch >= 'A' && ch <= 'Z') ch = shiftHeld ? ch : static_cast<char>(ch + 32);
                else if (shiftHeld && ch == '-') ch = '_';
                u.name.push_back(ch);
            }
        }
        static auto bsHoldStart = std::chrono::steady_clock::now();
        static auto bsLastRepeat = std::chrono::steady_clock::now();
        static bool bsWasHeld = false;
        bool bsHeld = keyDown(KEY_BACKSPACE);
#ifdef _WIN32
        if (GetAsyncKeyState(VK_BACK) & 0x8000) bsHeld = true;
#endif
        auto now = std::chrono::steady_clock::now();
        if (keyPressed(KEY_BACKSPACE))
        {
            if (!u.name.empty()) u.name.pop_back();
            bsHoldStart = now; bsLastRepeat = now; bsWasHeld = true;
        }
        else if (bsHeld)
        {
            if (!bsWasHeld) { if (!u.name.empty()) u.name.pop_back(); bsHoldStart = now; bsLastRepeat = now; bsWasHeld = true; }
            else if (std::chrono::duration_cast<std::chrono::milliseconds>(now - bsHoldStart).count() > 350 && std::chrono::duration_cast<std::chrono::milliseconds>(now - bsLastRepeat).count() > 45 && !u.name.empty())
            {
                u.name.pop_back();
                bsLastRepeat = now;
            }
        }
        else { bsWasHeld = false; }
        if (keyPressed(KEY_RETURN)) { renamingClass = false; statusBanner = "RENAMED DESIGN TO: " + u.name; }
    }
    iV_SetTextColour(pal_RGBA(110, 210, 255, 255)); iV_DrawText("UNIT CLASS EDITOR - GROUND-UP DESIGNER & SIMULATOR", 28, 42, font_large);
    iV_SetTextColour(pal_RGBA(200, 225, 245, 255)); iV_DrawText(statusBanner.c_str(), 28, 64, font_regular);
    drawBtn(102, 28, 76, 250, 28, renamingClass ? ("TYPE: " + u.name + "_") : ("NAME: " + u.name + " [EDIT]"), pal_RGBA(30, 62, 98, 250), pal_RGBA(120, 210, 255, 255));
    drawBtn(109, 286, 76, 156, 28, "SAVED DESIGNS (" + std::to_string(blueprints.size()) + ")", pal_RGBA(26, 56, 92, 245), pal_RGBA(105, 195, 255, 255));
    drawBtn(108, 450, 76, 114, 28, "CLEAR DESIGN", pal_RGBA(95, 52, 22, 245), pal_RGBA(255, 165, 80, 255));
    drawBtn(110, 570, 76, 68, 28, "UNDO", undoStack.empty() ? pal_RGBA(28, 34, 44, 220) : pal_RGBA(32, 68, 108, 245), pal_RGBA(110, 195, 255, 255));
    drawBtn(111, 644, 76, 68, 28, "REDO", redoStack.empty() ? pal_RGBA(28, 34, 44, 220) : pal_RGBA(32, 68, 108, 245), pal_RGBA(110, 195, 255, 255));
    drawBtn(105, sw - 420, 76, 128, 28, "SAVE DESIGN", pal_RGBA(24, 82, 52, 245), pal_RGBA(90, 240, 155, 255));
    drawBtn(107, sw - 284, 76, 160, 28, "LAUNCH SIMULATOR", pal_RGBA(120, 68, 16, 250), pal_RGBA(255, 195, 70, 255));
    drawBtn(106, sw - 116, 76, 88, 28, "EXIT", pal_RGBA(110, 28, 28, 245), pal_RGBA(255, 100, 100, 255));

    int lx = 28, lw = 310;
    pie_UniTransBoxFill(lx, 116, lx + lw, sh - 28, pal_RGBA(12, 20, 34, 240)); iV_Box(lx, 116, lx + lw, sh - 28, pal_RGBA(60, 140, 210, 220));
    iV_DrawText("1. CHASSIS (CLICK FOR 3D LIST)", lx + 12, 138, font_regular);
    drawBtn(201, lx + 12, 148, lw - 24, 26, u.bodyId.empty() ? "SELECT CHASSIS..." : ("HULL: " + u.bodyId), pal_RGBA(26, 52, 82, 240), pal_RGBA(90, 180, 255, 255));
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
    iV_DrawText("2. PROPULSION (CLICK FOR 3D LIST)", lx + 12, 330, font_regular);
    drawBtn(220, lx + 12, 340, lw - 24, 26, u.propId.empty() ? "SELECT PROPULSION..." : ("DRIVE: " + u.propId), pal_RGBA(26, 52, 82, 240), pal_RGBA(90, 180, 255, 255));
    drawBtn(221, lx + 12, 374, lw - 24, 26, u.matchChassisScale ? "MATCH CHASSIS SCALE: ON [AUTO]" : "MATCH CHASSIS SCALE: OFF [SYNC]", pal_RGBA(22, 84, 62, 245), pal_RGBA(95, 240, 175, 255));
    iV_DrawText(("UNIFORM SCALE: " + std::to_string(u.propScaleX).substr(0, 4) + "x").c_str(), lx + 12, 420, font_regular);
    drawBtn(225, lx + 180, 404, 52, 24, "-0.15", pal_RGBA(40, 45, 65, 240), pal_RGBA(120, 180, 240, 255));
    drawBtn(226, lx + 238, 404, 56, 24, "+0.15", pal_RGBA(40, 45, 65, 240), pal_RGBA(120, 180, 240, 255));
    iV_DrawText(("X:" + std::to_string(u.propScaleX).substr(0, 4) + " Y:" + std::to_string(u.propScaleY).substr(0, 4) + " Z:" + std::to_string(u.propScaleZ).substr(0, 4)).c_str(), lx + 12, 446, font_regular);
    drawBtn(227, lx + 12, 454, 90, 24, "WID +0.1", pal_RGBA(28, 48, 72, 240), pal_RGBA(90, 170, 240, 255));
    drawBtn(228, lx + 108, 454, 90, 24, "HGT +0.1", pal_RGBA(28, 48, 72, 240), pal_RGBA(90, 170, 240, 255));
    drawBtn(229, lx + 204, 454, 90, 24, "LEN +0.1", pal_RGBA(28, 48, 72, 240), pal_RGBA(90, 170, 240, 255));
    iV_DrawText(("DRV LIFT Y:" + std::to_string((int)u.propOffsetY) + " FORE Z:" + std::to_string((int)u.propOffsetZ)).c_str(), lx + 12, 498, font_regular);
    drawBtn(231, lx + 12, 506, 66, 24, "LIFT -2", pal_RGBA(35, 55, 80, 240), pal_RGBA(100, 190, 255, 255));
    drawBtn(232, lx + 84, 506, 66, 24, "LIFT +2", pal_RGBA(35, 55, 80, 240), pal_RGBA(100, 190, 255, 255));
    drawBtn(233, lx + 158, 506, 66, 24, "FORE -2", pal_RGBA(35, 55, 80, 240), pal_RGBA(100, 190, 255, 255));
    drawBtn(234, lx + 230, 506, 66, 24, "FORE +2", pal_RGBA(35, 55, 80, 240), pal_RGBA(100, 190, 255, 255));
    int estHp = u.bodyId.empty() ? 0 : static_cast<int>(450.0f * u.bodyScaleX * u.bodyScaleY * u.bodyScaleZ);
    iV_SetTextColour(pal_RGBA(120, 245, 165, 255)); iV_DrawText(("CLASS HP: " + std::to_string(estHp) + "   TURRETS: " + std::to_string(u.numTurrets)).c_str(), lx + 12, sh - 44, font_regular);

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
    int maxAllowed = (std::max({ u.bodyScaleX, u.bodyScaleY, u.bodyScaleZ }) >= 1.60f) ? 4 : 3;
    iV_DrawText(("TURRETS: " + std::to_string(u.numTurrets) + " / " + std::to_string(maxAllowed)).c_str(), rx + 12, 162, font_regular);
    drawBtn(401, rx + 180, 146, 52, 24, "-1", pal_RGBA(45, 45, 65, 240), pal_RGBA(120, 180, 240, 255));
    drawBtn(402, rx + 238, 146, 56, 24, "+1", pal_RGBA(45, 45, 65, 240), pal_RGBA(120, 180, 240, 255));
    for (int t = 0; t < 4; ++t) drawBtn(410 + t, rx + 10 + t * 73, 178, 69, 24, "M" + std::to_string(t + 1), (t == activeTurretSlot) ? pal_RGBA(35, 95, 145, 255) : pal_RGBA(22, 36, 58, 240), (t == activeTurretSlot) ? pal_RGBA(130, 230, 255, 255) : pal_RGBA(70, 130, 190, 220));
    auto& tr = u.turrets[activeTurretSlot];
    drawBtn(420, rx + 12, 212, lw - 24, 26, "WEAPON: " + tr.weaponId, pal_RGBA(26, 52, 82, 240), pal_RGBA(90, 180, 255, 255));
    drawBtn(421, rx + 12, 246, lw - 24, 26, std::string("PRESET: ") + kPresetNames[tr.presetIndex % 6], pal_RGBA(24, 76, 92, 245), pal_RGBA(100, 225, 255, 255));
    iV_DrawText(("TURRET SCALE: " + std::to_string(tr.scale).substr(0, 4) + "x").c_str(), rx + 12, 294, font_regular);
    drawBtn(422, rx + 180, 278, 52, 24, "-0.15", pal_RGBA(40, 45, 65, 240), pal_RGBA(120, 180, 240, 255));
    drawBtn(423, rx + 238, 278, 56, 24, "+0.15", pal_RGBA(40, 45, 65, 240), pal_RGBA(120, 180, 240, 255));
    iV_DrawText(("SNAP X: " + std::to_string((int)tr.snapX)).c_str(), rx + 12, 326, font_regular); drawBtn(430, rx + 180, 310, 52, 24, "X -3", pal_RGBA(35, 55, 80, 240), pal_RGBA(100, 190, 255, 255)); drawBtn(431, rx + 238, 310, 56, 24, "X +3", pal_RGBA(35, 55, 80, 240), pal_RGBA(100, 190, 255, 255));
    iV_DrawText(("SNAP Y: " + std::to_string((int)tr.snapY)).c_str(), rx + 12, 358, font_regular); drawBtn(432, rx + 180, 342, 52, 24, "Y -3", pal_RGBA(35, 55, 80, 240), pal_RGBA(100, 190, 255, 255)); drawBtn(433, rx + 238, 342, 56, 24, "Y +3", pal_RGBA(35, 55, 80, 240), pal_RGBA(100, 190, 255, 255));
    iV_DrawText(("SNAP Z: " + std::to_string((int)tr.snapZ)).c_str(), rx + 12, 390, font_regular); drawBtn(434, rx + 180, 374, 52, 24, "Z -3", pal_RGBA(35, 55, 80, 240), pal_RGBA(100, 190, 255, 255)); drawBtn(435, rx + 238, 374, 56, 24, "Z +3", pal_RGBA(35, 55, 80, 240), pal_RGBA(100, 190, 255, 255));
    if (pickerMode == 1 || pickerMode == 2) {
        int gx = 160, gy = 116, gw = sw - 320, gh = sh - 150;
        pie_UniTransBoxFill(gx, gy, gx + gw, gy + gh, pal_RGBA(8, 16, 28, 252)); iV_Box(gx, gy, gx + gw, gy + gh, pal_RGBA(110, 215, 255, 255));
        iV_SetTextColour(pal_RGBA(130, 225, 255, 255)); iV_DrawText(pickerMode == 1 ? "SELECT CHASSIS MODEL (3D RENDERED GALLERY)" : "SELECT PROPULSION SYSTEM (3D RENDERED GALLERY)", gx + 18, gy + 26, font_large);
        drawBtn(699, gx + gw - 110, gy + 8, 96, 26, "CLOSE [X]", pal_RGBA(115, 28, 28, 250), pal_RGBA(255, 110, 110, 255));
        int count = (pickerMode == 1) ? 10 : 5, cols = 5, cw = (gw - 24 - (cols - 1) * 10) / cols, ch = (pickerMode == 1) ? 165 : 210;
        for (int idx = 0; idx < count; ++idx) {
            int col = idx % cols, row = idx / cols, cxCard = gx + 12 + col * (cw + 10), cyCard = gy + 44 + row * (ch + 12);
            std::string itemId = (pickerMode == 1) ? kBodies[idx] : kProps[idx], itemName = (pickerMode == 1) ? kBodyNames[idx] : kPropNames[idx];
            bool isSel = (pickerMode == 1) ? (u.bodyId == itemId) : (u.propId == itemId);
            clickBoxes.push_back({ (pickerMode == 1 ? 600 : 650) + idx, cxCard, cyCard, cw, ch });
            pie_UniTransBoxFill(cxCard, cyCard, cxCard + cw, cyCard + ch, isSel ? pal_RGBA(22, 72, 108, 245) : pal_RGBA(14, 26, 44, 240));
            iV_Box(cxCard, cyCard, cxCard + cw, cyCard + ch, isSel ? pal_RGBA(110, 245, 175, 255) : pal_RGBA(65, 145, 215, 230));
            int cIdx = getCompFromID(pickerMode == 1 ? COMP_BODY : COMP_PROPULSION, WzString::fromUtf8(itemId));
            if (cIdx >= 0) {
                const iIMDBaseShape* bSh = (pickerMode == 1) ? asBodyStats[cIdx].pIMD : asPropulsionStats[cIdx].pIMD;
                if (bSh) {
                    static float sGalRot = 0.0f; sGalRot = std::fmod(sGalRot + 0.045f, 360.0f);
                    const iIMDShape* rootImd = bSh->displayModel();
                    float rad = (rootImd && rootImd->radius > 10) ? static_cast<float>(rootImd->radius) : 65.0f;
                    float fitFactor = std::min(1.45f, std::max(0.58f, 78.0f / rad));
                    pie_Begin3DScene(); pie_SetGeometricOffset(cxCard + cw / 2, cyCard + ch / 2 - 4);
                    glm::mat4 m = glm::translate(glm::vec3(0.f, 0.f, 680.f)) * glm::rotate(-0.40f, glm::vec3(1.f, 0.f, 0.f)) * glm::rotate(sGalRot * 3.14159265f / 180.f, glm::vec3(0.f, 1.f, 0.f)) * glm::scale(glm::vec3((DROID_BUT_SCALE * fitFactor) / 100.f));
                    for (const iIMDShape* imd = rootImd; imd != nullptr; imd = imd->next.get()) pie_Draw3DButton(imd, imd->getTeamColourForModel(getPlayerColour(0)), m, glm::mat4(1.f));
                    pie_BeginInterface(); pie_SetGeometricOffset(pie_GetVideoBufferWidth() / 2, pie_GetVideoBufferHeight() / 2);
                }
            }
            iV_SetTextColour(WZCOL_WHITE); iV_DrawText(itemName.c_str(), cxCard + 8, cyCard + ch - 24, font_regular);
            iV_SetTextColour(pal_RGBA(150, 200, 240, 230)); iV_DrawText(itemId.c_str(), cxCard + 8, cyCard + ch - 8, font_regular);
        }
    }
    else if (pickerMode == 3)
    {
        int gx = 72, gy = 118, gw = sw - 144, gh = sh - 156;
        pie_UniTransBoxFill(gx, gy, gx + gw, gy + gh, pal_RGBA(8, 16, 28, 252)); iV_Box(gx, gy, gx + gw, gy + gh, pal_RGBA(110, 215, 255, 255));
        iV_SetTextColour(pal_RGBA(130, 225, 255, 255)); iV_DrawText("SAVED UNIT DESIGNS (CLICK CARD TO LOAD | RED [X] TO DELETE)", gx + 18, gy + 28, font_large);
        drawBtn(699, gx + gw - 110, gy + 8, 96, 26, "CLOSE [X]", pal_RGBA(115, 28, 28, 250), pal_RGBA(255, 110, 110, 255));
        if (blueprints.empty())
        {
            iV_SetTextColour(pal_RGBA(220, 200, 130, 255));
            iV_DrawText("NO SAVED DESIGNS YET - BUILD A DESIGN AND CLICK [SAVE DESIGN] FIRST.", gx + 24, gy + 82, font_regular);
        }
        else
        {
            static float sCardRot = 0.0f; sCardRot = std::fmod(sCardRot + 0.35f, 360.0f); float cardYaw = sCardRot * 3.14159265f / 180.0f;
            int cols = 2, cw = (gw - 42) / cols, ch = 152;
            for (size_t i = 0; i < blueprints.size() && i < 6; ++i)
            {
                const auto& sb = blueprints[i];
                int col = static_cast<int>(i) % cols, row = static_cast<int>(i) / cols;
                int cx = gx + 14 + col * (cw + 14), cy = gy + 44 + row * (ch + 12);
                clickBoxes.push_back({ 700 + static_cast<int>(i), cx, cy, cw - 34, ch });
                pie_UniTransBoxFill(cx, cy, cx + cw, cy + ch, pal_RGBA(14, 28, 48, 245));
                iV_Box(cx, cy, cx + cw, cy + ch, pal_RGBA(75, 165, 235, 240));
                int pvW = 148, pvH = ch - 16, pvCx = cx + 8 + pvW / 2, pvCy = cy + 8 + pvH / 2 + 16;
                pie_UniTransBoxFill(cx + 8, cy + 8, cx + 8 + pvW, cy + ch - 8, pal_RGBA(6, 12, 22, 250));
                iV_Box(cx + 8, cy + 8, cx + 8 + pvW, cy + ch - 8, pal_RGBA(45, 110, 165, 210));
                int bIdx = getCompFromID(COMP_BODY, WzString::fromUtf8(sb.bodyId)), pIdx = getCompFromID(COMP_PROPULSION, WzString::fromUtf8(sb.propId));
                float maxSc = std::max({ 1.35f, sb.bodyScaleX, sb.bodyScaleY * 1.15f, sb.bodyScaleZ, sb.propScaleX, sb.propScaleY * 1.15f, sb.propScaleZ });
                float fitSc = (DROID_BUT_SCALE * 0.54f) / (100.0f * maxSc);
                pie_Begin3DScene(); pie_SetGeometricOffset(pvCx, pvCy);
                glm::mat4 rMat = glm::translate(glm::vec3(0.f, 0.f, 680.f)) * glm::rotate(-0.38f, glm::vec3(1.f, 0.f, 0.f)) * glm::rotate(cardYaw, glm::vec3(0.f, 1.f, 0.f)) * glm::scale(glm::vec3(fitSc));
                auto drawMini = [&](const iIMDBaseShape* bSh, const glm::mat4& m) { if (!bSh) return; for (const iIMDShape* imd = bSh->displayModel(); imd != nullptr; imd = imd->next.get()) pie_Draw3DButton(imd, imd->getTeamColourForModel(getPlayerColour(0)), m, glm::mat4(1.f)); };
                if (pIdx >= 0) { glm::mat4 pm = rMat * glm::translate(glm::vec3(0.f, sb.propOffsetY, sb.propOffsetZ)) * glm::scale(glm::vec3(sb.propScaleX, sb.propScaleY, sb.propScaleZ)); if (bIdx >= 0 && !asBodyStats[bIdx].ppIMDList.empty()) { drawMini(asBodyStats[bIdx].ppIMDList[pIdx * NUM_PROP_SIDES + LEFT_PROP], pm); drawMini(asBodyStats[bIdx].ppIMDList[pIdx * NUM_PROP_SIDES + RIGHT_PROP], pm); } else drawMini(asPropulsionStats[pIdx].pIMD, pm); }
                if (bIdx >= 0) drawMini(asBodyStats[bIdx].pIMD, rMat * glm::translate(glm::vec3(0.f, sb.bodyOffsetY, sb.bodyOffsetZ)) * glm::scale(glm::vec3(sb.bodyScaleX, sb.bodyScaleY, sb.bodyScaleZ)));
                for (int t = 0; t < std::min(4, sb.numTurrets); ++t) { int wi = getCompFromID(COMP_WEAPON, WzString::fromUtf8(sb.turrets[t].weaponId)); if (wi >= 0) { glm::mat4 tm = rMat * glm::translate(glm::vec3(sb.turrets[t].snapX, sb.bodyOffsetY + sb.turrets[t].snapY, sb.bodyOffsetZ + sb.turrets[t].snapZ)) * glm::scale(glm::vec3(sb.turrets[t].scale)); drawMini(asWeaponStats[wi].pMountGraphic, tm); drawMini(asWeaponStats[wi].pIMD, tm); } }
                pie_BeginInterface(); pie_SetGeometricOffset(pie_GetVideoBufferWidth() / 2, pie_GetVideoBufferHeight() / 2);
                int tx = cx + pvW + 18, hp = static_cast<int>(450.0f * sb.bodyScaleX * sb.bodyScaleY * sb.bodyScaleZ);
                iV_SetTextColour(pal_RGBA(120, 230, 255, 255)); iV_DrawText(sb.name.c_str(), tx, cy + 26, font_large);
                iV_SetTextColour(WZCOL_WHITE); iV_DrawText(("HULL: " + sb.bodyId + "   DRIVE: " + sb.propId).c_str(), tx, cy + 50, font_regular);
                iV_SetTextColour(pal_RGBA(120, 245, 165, 255)); iV_DrawText(("EST. HP: " + std::to_string(hp) + "   SCALE: " + std::to_string(sb.bodyScaleX).substr(0, 4) + "x").c_str(), tx, cy + 72, font_regular);
                std::string wList = "TURRETS (" + std::to_string(sb.numTurrets) + "): "; for (int t = 0; t < std::min(4, sb.numTurrets); ++t) wList += sb.turrets[t].weaponId + (t + 1 < sb.numTurrets ? ", " : "");
                iV_SetTextColour(pal_RGBA(225, 205, 130, 255)); iV_DrawText((wList.size() > 34 ? (wList.substr(0, 31) + "...") : wList).c_str(), tx, cy + 94, font_regular);
                iV_SetTextColour(pal_RGBA(145, 195, 240, 230)); iV_DrawText("CLICK CARD TO LOAD INTO EDITOR", tx, cy + ch - 14, font_regular);
                drawBtn(750 + static_cast<int>(i), cx + cw - 30, cy + 6, 24, 24, "X", pal_RGBA(155, 24, 24, 250), pal_RGBA(255, 95, 95, 255));
            }
        }
        if (deleteConfirmIdx >= 0 && deleteConfirmIdx < static_cast<int>(blueprints.size()))
        {
            int mw = 640, mh = 146, mx0 = (sw - mw) / 2, my0 = (sh - mh) / 2;
            pie_UniTransBoxFill(mx0, my0, mx0 + mw, my0 + mh, pal_RGBA(18, 10, 14, 254));
            iV_Box(mx0, my0, mx0 + mw, my0 + mh, pal_RGBA(255, 90, 90, 255));
            iV_SetTextColour(pal_RGBA(255, 130, 130, 255)); iV_DrawText("DELETE SAVED DESIGN - ARE YOU SURE?", mx0 + 24, my0 + 34, font_large);
            iV_SetTextColour(WZCOL_WHITE); iV_DrawText(("DESIGN: " + blueprints[deleteConfirmIdx].name).c_str(), mx0 + 24, my0 + 62, font_regular);
            drawBtn(790, mx0 + 48, my0 + 94, 240, 30, "YES, DELETE", pal_RGBA(150, 24, 24, 250), pal_RGBA(255, 105, 105, 255));
            drawBtn(791, mx0 + 352, my0 + 94, 240, 30, "CANCEL", pal_RGBA(32, 54, 82, 245), pal_RGBA(110, 195, 255, 255));
        }
    }
}
bool usaHandleUnitEditorClick(int mx, int my, bool rightClick)
{
    if (!editorOpen) return false;
    SuperHeavyBlueprint& u = activeDesign; auto& tr = u.turrets[activeTurretSlot];
    float dir = rightClick ? -1.0f : 1.0f;
    auto syncScaleRules = [&]() {
        int maxT = (std::max({ u.bodyScaleX, u.bodyScaleY, u.bodyScaleZ }) >= 1.60f) ? 4 : 3;
        if (maxT == 4 && u.numTurrets == 3) u.numTurrets = 4;
        if (u.numTurrets > maxT) u.numTurrets = maxT;
        activeTurretSlot = std::min(activeTurretSlot, std::max(0, u.numTurrets - 1));
        if (u.matchChassisScale) { u.propScaleX = u.bodyScaleX; u.propScaleY = std::max(1.0f, u.bodyScaleY * 0.94f); u.propScaleZ = u.bodyScaleZ; u.bodyOffsetY = (u.propScaleY - 1.0f) * 12.0f; u.propOffsetY = 0.0f; u.propOffsetZ = 0.0f; }
    };
    for (const auto& b : clickBoxes) {
        if (mx < b.x || mx > b.x + b.w || my < b.y || my > b.y + b.h) continue;
        if (b.id == 110) { doUndo(); return true; }
        if (b.id == 111) { doRedo(); return true; }
        if (b.id == 108 || b.id >= 202) pushUndoState();
        switch (b.id) {
        case 102: if (!renamingClass) pushUndoState(); renamingClass = !renamingClass; if (renamingClass) inputClearBuffer(); statusBanner = renamingClass ? "TYPE DESIGN NAME ON KEYBOARD (ENTER TO CONFIRM)" : ("CONFIRMED: " + u.name); break;
        case 109: pickerMode = (pickerMode == 3) ? 0 : 3; statusBanner = blueprints.empty() ? "NO SAVED DESIGNS YET" : "SELECT A SAVED DESIGN TO LOAD"; break;
        case 108: resetBlankDesign(); pickerMode = 0; statusBanner = "CLEARED DESIGN TO BLANK CANVAS"; break;
        case 105: { if (u.bodyId.empty() || u.propId.empty()) { statusBanner = "CANNOT SAVE BLANK DESIGN - PICK CHASSIS & PROPULSION FIRST"; break; } if (u.numTurrets <= 0) u.numTurrets = 1; bool upd = false; for (auto& ex : blueprints) if (ex.name == u.name) { ex = u; upd = true; break; } if (!upd) blueprints.push_back(u); usaSaveSuperHeavyBlueprints(); statusBanner = "SAVED DESIGN: " + u.name + " (" + std::to_string(blueprints.size()) + " TOTAL)"; break; }
        case 107: { if (u.bodyId.empty() || u.propId.empty()) { statusBanner = "SELECT & SAVE A DESIGN BEFORE LAUNCHING SIMULATOR!"; break; } if (u.numTurrets <= 0) u.numTurrets = 1; bool upd = false; for (auto& ex : blueprints) if (ex.name == u.name) { ex = u; upd = true; break; } if (!upd) blueprints.push_back(u); usaSaveSuperHeavyBlueprints(); usaSimulatorClassName = u.name; usaCloseUnitEditor(); usaLaunchSimulatorArena(); return true; }
        case 106: usaCloseUnitEditor(); break;
        case 201: pickerMode = (pickerMode == 1) ? 0 : 1; statusBanner = "SELECT CHASSIS FROM 3D GALLERY"; break;
        case 202: u.bodyScaleX = std::max(1.25f, std::min(3.0f, u.bodyScaleX - 0.15f * dir)); u.bodyScaleY = std::max(1.25f, std::min(3.0f, u.bodyScaleY - 0.15f * dir)); u.bodyScaleZ = std::max(1.25f, std::min(3.0f, u.bodyScaleZ - 0.15f * dir)); syncScaleRules(); break;
        case 203: u.bodyScaleX = std::max(1.25f, std::min(3.0f, u.bodyScaleX + 0.15f * dir)); u.bodyScaleY = std::max(1.25f, std::min(3.0f, u.bodyScaleY + 0.15f * dir)); u.bodyScaleZ = std::max(1.25f, std::min(3.0f, u.bodyScaleZ + 0.15f * dir)); syncScaleRules(); break;
        case 204: u.bodyScaleX = std::max(1.0f, std::min(3.0f, u.bodyScaleX + 0.10f * dir)); syncScaleRules(); break;
        case 205: u.bodyScaleY = std::max(1.0f, std::min(3.0f, u.bodyScaleY + 0.10f * dir)); syncScaleRules(); break;
        case 206: u.bodyScaleZ = std::max(1.0f, std::min(3.0f, u.bodyScaleZ + 0.10f * dir)); syncScaleRules(); break;
        case 207: u.bodyOffsetY -= 2.0f * dir; break; case 208: u.bodyOffsetY += 2.0f * dir; break; case 209: u.bodyOffsetZ -= 2.0f * dir; break; case 210: u.bodyOffsetZ += 2.0f * dir; break;
        case 220: pickerMode = (pickerMode == 2) ? 0 : 2; statusBanner = "SELECT PROPULSION FROM 3D GALLERY"; break;
        case 221: u.matchChassisScale = !u.matchChassisScale; if (u.matchChassisScale) syncScaleRules(); break;
        case 225: u.matchChassisScale = false; u.propScaleX = std::max(1.0f, std::min(3.0f, u.propScaleX - 0.15f * dir)); u.propScaleY = std::max(1.0f, std::min(3.0f, u.propScaleY - 0.15f * dir)); u.propScaleZ = std::max(1.0f, std::min(3.0f, u.propScaleZ - 0.15f * dir)); break;
        case 226: u.matchChassisScale = false; u.propScaleX = std::max(1.0f, std::min(3.0f, u.propScaleX + 0.15f * dir)); u.propScaleY = std::max(1.0f, std::min(3.0f, u.propScaleY + 0.15f * dir)); u.propScaleZ = std::max(1.0f, std::min(3.0f, u.propScaleZ + 0.15f * dir)); break;
        case 227: u.matchChassisScale = false; u.propScaleX = std::max(1.0f, std::min(3.0f, u.propScaleX + 0.10f * dir)); break;
        case 228: u.matchChassisScale = false; u.propScaleY = std::max(1.0f, std::min(3.0f, u.propScaleY + 0.10f * dir)); break;
        case 229: u.matchChassisScale = false; u.propScaleZ = std::max(1.0f, std::min(3.0f, u.propScaleZ + 0.10f * dir)); break;
        case 231: u.propOffsetY -= 2.0f * dir; break; case 232: u.propOffsetY += 2.0f * dir; break; case 233: u.propOffsetZ -= 2.0f * dir; break; case 234: u.propOffsetZ += 2.0f * dir; break;
        case 301: autoSpin = false; previewYaw -= 18.0f * dir; break; case 302: autoSpin = !autoSpin; break; case 303: autoSpin = false; previewYaw += 18.0f * dir; break;
        case 304: previewZoom = std::max(0.55f, std::min(1.85f, previewZoom - 0.15f * dir)); break; case 305: previewZoom = std::max(0.55f, std::min(1.85f, previewZoom + 0.15f * dir)); break;
        case 401: u.numTurrets = std::max(0, std::min(4, u.numTurrets - (rightClick ? -1 : 1))); activeTurretSlot = std::min(activeTurretSlot, std::max(0, u.numTurrets - 1)); break;
        case 402: { int maxT = (std::max({ u.bodyScaleX, u.bodyScaleY, u.bodyScaleZ }) >= 1.60f) ? 4 : 3; u.numTurrets = std::max(0, std::min(maxT, u.numTurrets + (rightClick ? -1 : 1))); break; }
        case 410: case 411: case 412: case 413: activeTurretSlot = b.id - 410; break;
        case 420: { int idx = 0; for (int i = 0; i < 9; ++i) if (tr.weaponId == kWeapons[i]) { idx = rightClick ? ((i + 8) % 9) : ((i + 1) % 9); break; } tr.weaponId = kWeapons[idx]; statusBanner = std::string("WEAPON: ") + kWeapNames[idx]; break; }
        case 421: tr.presetIndex = (tr.presetIndex + (rightClick ? 5 : 1)) % 6; applyTurretPreset(u, activeTurretSlot); statusBanner = std::string("PRESET: ") + kPresetNames[tr.presetIndex]; break;
        case 422: tr.scale = std::max(0.85f, std::min(2.50f, tr.scale - 0.15f * dir)); break;
        case 423: tr.scale = std::max(0.85f, std::min(2.50f, tr.scale + 0.15f * dir)); break;
        case 430: tr.snapX -= 3.0f * dir; break; case 431: tr.snapX += 3.0f * dir; break;
        case 432: tr.snapY -= 3.0f * dir; break; case 433: tr.snapY += 3.0f * dir; break;
        case 434: tr.snapZ -= 3.0f * dir; break; case 435: tr.snapZ += 3.0f * dir; break;
        case 500: case 501: case 502: case 503: case 504: case 505: case 506: case 507: case 508: { int ni = b.id - 500; if (activeTurretSlot >= u.numTurrets) u.numTurrets = activeTurretSlot + 1; auto& sr = u.turrets[activeTurretSlot]; sr.snapX = kHullNodes[ni].nx * 15.0f * u.bodyScaleX; sr.snapY = kHullNodes[ni].ny * 11.0f * u.bodyScaleY; sr.snapZ = kHullNodes[ni].nz * 17.0f * u.bodyScaleZ; statusBanner = std::string("MOUNT ") + std::to_string(activeTurretSlot + 1) + " SNAPPED TO " + kHullNodes[ni].label; break; }
        case 699: deleteConfirmIdx = -1; pickerMode = 0; break; case 790: if (deleteConfirmIdx >= 0 && deleteConfirmIdx < static_cast<int>(blueprints.size())) { statusBanner = "DELETED DESIGN: " + blueprints[deleteConfirmIdx].name; blueprints.erase(blueprints.begin() + deleteConfirmIdx); usaSaveSuperHeavyBlueprints(); } deleteConfirmIdx = -1; break; case 791: deleteConfirmIdx = -1; break;
        case 600: case 601: case 602: case 603: case 604: case 605: case 606: case 607: case 608: case 609: { int idx = b.id - 600; u.bodyId = kBodies[idx]; pickerMode = 0; statusBanner = std::string("EQUIPPED HULL: ") + kBodyNames[idx]; break; }
        case 650: case 651: case 652: case 653: case 654: { int idx = b.id - 650; u.propId = kProps[idx]; pickerMode = 0; syncScaleRules(); statusBanner = std::string("EQUIPPED DRIVE: ") + kPropNames[idx]; break; }
        default: if (b.id >= 750 && b.id < 750 + static_cast<int>(blueprints.size())) { deleteConfirmIdx = b.id - 750; } else if (deleteConfirmIdx < 0 && b.id >= 700 && b.id < 700 + static_cast<int>(blueprints.size())) { activeDesign = blueprints[b.id - 700]; activeTurretSlot = 0; pickerMode = 0; statusBanner = "LOADED SAVED DESIGN: " + activeDesign.name; } break;
        }
        return true;
    }
    return true;
}
void usaLaunchSimulatorArena()
{
    bUsaSimulatorMode = true; bUsaPrebattle = false; bUsaStripVanillaLobby = true; bUsaSessionActive = true;
    std::string jStr = "{\n  \"challenge\": {\n    \"name\": \"Unit Class Simulator Arena\",\n    \"map\": \"Sk-Rush\",\n    \"maxPlayers\": 2,\n    \"scavengers\": false,\n    \"allowPositionChange\": false,\n    \"bases\": 0,\n    \"powerLevel\": 2,\n    \"alliances\": 0,\n    \"techLevel\": 3,\n    \"player_0\": { \"name\": \"The Project\", \"team\": 0, \"position\": 0, \"colour\": 0 },\n    \"player_1\": { \"name\": \"Simulator Range\", \"team\": 1, \"position\": 1, \"colour\": 7, \"ai\": \"null\" }\n  }\n}\n";
    PHYSFS_mkdir("tests"); if (PHYSFS_File* f = PHYSFS_openWrite("tests/usa.json")) { PHYSFS_writeBytes(f, jStr.data(), jStr.size()); PHYSFS_close(f); }
    std::string bStr = "{\n  \"stateName\": \"COMBAT TEST ARENA (SK-RUSH)\",\n  \"factionName\": \"Simulator Range\",\n  \"mapName\": \"Sk-Rush\",\n  \"mode\": \"simulator\",\n  \"turn\": 1,\n  \"power\": 50000\n}\n";
    if (PHYSFS_File* bf = PHYSFS_openWrite("tests/usa_briefing.json")) { PHYSFS_writeBytes(bf, bStr.data(), bStr.size()); PHYSFS_close(bf); }
    usaSetSkirmishTestName("usa.json"); challengeName = "usa.json";
    setHostLaunch(HostLaunch::Skirmish); SPinit(LEVEL_TYPE::SKIRMISH);
    sstrcpy(game.map, "Sk-Rush"); game.maxPlayers = 2; game.base = CAMP_CLEAN;
    bMultiPlayer = true; ingame.side = InGameSide::HOST_OR_SINGLEPLAYER; game.type = LEVEL_TYPE::SKIRMISH;
    changeTitleMode(TITLE); changeTitleUI(std::make_shared<WzMultiplayerOptionsTitleUI>(wzTitleUICurrent));
}
