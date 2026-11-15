# Warzone 2100: USA Reclamation Initiative (Standalone RTW Fork)

## Repository & Architecture
- **Repo:** https://github.com
- **Local Path:** C:\Games\warzone2100_source (Build output: build\src\warzone2100.exe)

## Completed Milestones
1. **`src/usa_campaign_state.h/.cpp` & `src/usa_overworld_ui.cpp` (Pre-Map Player Setup Screen)**:
   - Opens **Strategic Command Faction Initialization** right before the USA Risk Map launches (and whenever starting a New Campaign).
   - **Custom Faction Name**: Direct keyboard typing + quick-select presets, reflected live across the Map Legend, Sector Panel, and save file (`PLAYER_FACTION`).
   - **8 Exclusive Player Colors**: Reserved palette (`Cobalt Blue`, `Emerald Cyan`, `Crimson Red`, `Royal Violet`, `Arctic Gold`, `Titanium Silver`, `Deep Indigo`, `Teal Horizon`) guaranteed never to be used by Neo Paradigm or any of the 12 Scavenger Warlord clusters.
2. **`src/usa_overworld_ui.cpp` & `src/usa_campaign_state.h/.cpp` (Continental USA Faction Map & Auto-Sizing Dynamic Legend)**:
   - Player starts in **California (`CA`)** (`1` sector), **Neo Paradigm** starts in **Texas (`TX`)** (`1` sector), and the remaining 46 states are grouped into **12 four-state Scavenger Warlord clusters**.
   - Auto-sizing 5-column Dynamic Map Legend, retro ocean CRT scanlines & sonar rings, continuous radar scan sweep, flush neon-glow state outlines, and unobstructed state initial badges.
3. **`src/design.cpp`, `src/usa_metagame_bridge.cpp` & `src/usa_campaign_state.cpp` (Exclusive Hero General 3D Designer)**:
   - Native 3D Unit Design Screen hooked directly into the Overworld Risk Map with blank-start Dragon Command General build, starting catalog (`Dragon` + `Wheels` + `Heavy Machinegun` / `Medium Cannon` / `Mini-Rocket Pod`), and isolated `SAVED GENERAL VARIANTS` panel.
4. **`src/usa_battle_setup.h/.cpp`, `src/frontend.cpp` & `src/clparse.cpp`**:
   - Custom 2-button Main Menu, Western Scav Syndicate faction registration, Dragon Command General deployment, and structured Scavenger wave pacing.

## Current Milestone
- Compile and test the **Player Faction Name & Exclusive Color Setup Screen** before the USA Risk Map launches, then push to GitHub.