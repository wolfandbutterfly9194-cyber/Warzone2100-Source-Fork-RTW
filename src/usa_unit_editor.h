#pragma once
#include <string>
#include <vector>
#include <cstdint>
struct SuperHeavyTurretSlot { std::string weaponId; float scale = 1.15f, snapX = 0.0f, snapY = 10.0f, snapZ = 0.0f; int presetIndex = 0; };
struct SuperHeavyBlueprint {
    std::string name = "New Unit Class", bodyId = "", propId = "";
    float bodyScaleX = 1.35f, bodyScaleY = 1.35f, bodyScaleZ = 1.35f, bodyOffsetY = 5.0f, bodyOffsetZ = 0.0f;
    bool matchChassisScale = true;
    float propScaleX = 1.35f, propScaleY = 1.28f, propScaleZ = 1.35f, propOffsetY = 0.0f, propOffsetZ = 0.0f;
    int numTurrets = 0;
    SuperHeavyTurretSlot turrets[4];
};
void usaOpenUnitEditor(); void usaCloseUnitEditor(); bool usaIsUnitEditorOpen();
void usaUpdateAndDrawUnitEditor(); bool usaHandleUnitEditorClick(int mx, int my, bool rightClick = false);
const std::vector<SuperHeavyBlueprint>& usaGetSuperHeavyBlueprints();
const SuperHeavyBlueprint* usaFindSuperHeavyByName(const std::string& name);
void usaSaveSuperHeavyBlueprints(); void usaLoadSuperHeavyBlueprints(); void usaRegisterSuperHeavyTemplatesForPlayer(int player);
extern bool bUsaSimulatorMode; extern std::string usaSimulatorClassName; void usaLaunchSimulatorArena();
