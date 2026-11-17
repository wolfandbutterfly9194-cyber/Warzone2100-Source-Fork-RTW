#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct SuperHeavyTurretSlot
{
    std::string weaponId;
    float scale = 1.0f;
    float snapX = 0.0f;
    float snapY = 0.0f;
    float snapZ = 0.0f;
    int presetIndex = 0;
};

struct SuperHeavyBlueprint
{
    std::string name = "Dragon-X Behemoth";
    std::string bodyId = "Body12SUP";
    std::string propId = "tracked01";
    float bodyScaleX = 1.35f;
    float bodyScaleY = 1.35f;
    float bodyScaleZ = 1.35f;
    float bodyOffsetY = 6.0f;
    float bodyOffsetZ = 0.0f;

    bool matchChassisScale = true;
    float propScaleX = 1.35f;
    float propScaleY = 1.30f;
    float propScaleZ = 1.35f;
    float propOffsetY = 0.0f;
    float propOffsetZ = 0.0f;

    int numTurrets = 3;
    SuperHeavyTurretSlot turrets[4];
};

void usaOpenUnitEditor();
void usaCloseUnitEditor();
bool usaIsUnitEditorOpen();
void usaUpdateAndDrawUnitEditor();
bool usaHandleUnitEditorClick(int mx, int my, bool rightClick = false);

const std::vector<SuperHeavyBlueprint>& usaGetSuperHeavyBlueprints();
const SuperHeavyBlueprint* usaFindSuperHeavyByName(const std::string& name);
void usaSaveSuperHeavyBlueprints();
void usaLoadSuperHeavyBlueprints();
void usaRegisterSuperHeavyTemplatesForPlayer(int player);
extern bool bUsaSimulatorMode;
extern std::string usaSimulatorClassName;
void usaLaunchSimulatorArena();
