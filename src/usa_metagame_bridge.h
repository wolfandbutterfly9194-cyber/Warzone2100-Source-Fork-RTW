#pragma once

#define USA_RESTORE_VANILLA_SKIRMISH 0

extern bool bUsaPrebattle;
extern bool bUsaStripVanillaLobby;
extern bool bUsaSessionActive;
enum class UsaOverworldDesignMode { NONE = 0, CAMPAIGN_DESIGN = 1, GENERAL_DESIGN = 2 };
extern UsaOverworldDesignMode usaOverworldDesignMode;
void usaOpenOverworldDesigner(UsaOverworldDesignMode mode);
void usaCloseOverworldDesigner();
extern bool bUsaGeneralDesignMode;
extern bool bUsaExitDesignToMap;

void startUsaPrebattleScreen();
void runUsaPrebattleTick();
void updateUsaInGameHud();
