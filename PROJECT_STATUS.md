# Warzone 2100: USA Reclamation Initiative (Standalone RTW Fork)

## Repository & Architecture
- **Repo:** https://github.com
- **Local Path:** C:\Games\warzone2100_source (Build output: build\src\warzone2100.exe)

## Completed Milestones
1. **`src/usa_campaign_state.h/.cpp`**: 48-state continental USA Risk map, faction ownership, power economy, research upgrades, unit designer catalog, and PHYSFS save/load (`usa_campaign/usa_campaign_save.txt`).
2. **`src/usa_battle_setup.h/.cpp`**: Registered **Western Scav Syndicate** as a full native faction (`usaUnlockScavFactionForPlayer` + `usaGetScavFactionTemplates`) with `validLocation(...)` spiral citadel placement, unified faction colors (`setPlayerColour`), and `addDroid(psNew, apsDroidLists)` spawning.
3. **`src/usa_overworld_ui.h/.cpp`**: OpenGL/SDL3 interactive 48-state USA Risk Map overworld UI rendered directly inside Warzone 2100.
4. **`src/frontend.cpp` & `src/clparse.cpp`**: Custom 2-button Main Menu (`Global Reclamation Campaign` + `Options`) entering the Risk Map directly with zero external launchers.
5. **`src/usa_metagame_bridge.cpp`**: Structured Scavenger wave pacing (`Wave 1: 65 cap`, `Wave 2: 85 cap`, `Wave 3: 105 cap` up to `140`), **18-tile** close-perimeter forward rally staging for **150s (2m 30s)**, and **600s (10m)** sustained Mass Assault windows with continuous attack orders.

## Current Milestone & Next Steps
- **Verified Working:** Full loop from 2-button Main Menu -> 48-state USA Risk Map -> Scav invasion with playable Scav faction, 18-tile staging, 2m 30s rally buildup, and 10m Mass Assault waves.
- **Next Feature:** Polish post-battle victory/defeat auto-return to the 48-state USA Risk Map (automatically awarding territory capture & income on victory) and expand enemy AI counter-attacks between turns.