# Warzone 2100: USA Reclamation Initiative (Standalone RTW Fork)

## Repository & Architecture
- **Repo:** https://github.com
- **Local Path:** C:\Games\warzone2100_source (Build output: build\src\warzone2100.exe)

## Integrated C++ Modules (`src/`)
1. **`src/usa_campaign_state.h/.cpp`**: Holds the 48-state continental USA Risk map, faction ownership, power income, research upgrades, unit designer catalog, and PHYSFS save/load (`usa_campaign/usa_campaign_save.txt`).
2. **`src/usa_battle_setup.h/.cpp`**: Native C++ mission initializer (`usaApplyNativeBattleSetup`) spawning enemy citadels/factories, player saved bases, tech unlocks, and veteran squads, plus post-battle export (`usaExportSessionResults`).
3. **`src/usa_overworld_ui.h/.cpp`**: OpenGL/SDL3 interactive 48-state USA Risk Map overworld UI rendered directly inside Warzone 2100.
4. **`src/frontend.cpp` & `src/clparse.cpp`**: Custom 2-button Main Menu (`Global Reclamation Campaign` + `Options`) that enters the Risk Map directly and launches `tests/usa.json` skirmishes without external launchers.
5. **`src/usa_metagame_bridge.cpp`**: In-game HUD (`OPFOR IN M:SS | FACS: X | ENEMIES: Y`), `[ ATTACK NOW (F9) ]` button, 42-tile forward Scav rally staging, synchronized mass waves, and 6-second factory rollouts.

## Current Milestone & Next Step
- Verify end-to-end flow: Main Menu -> `Global Reclamation Campaign` -> 48-state USA Risk Map -> `Enter Base Build` / `Launch Tactical Invasion` -> 3D battle -> `Save & Return`.