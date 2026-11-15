# Warzone 2100: USA Reclamation Initiative (Standalone RTW Fork)

## Repository & Architecture
- **Repo:** https://github.com
- **Local Path:** C:\Games\warzone2100_source (Build output: build\src\warzone2100.exe)

## Completed Milestones
1. **`src/usa_overworld_ui.cpp` & `src/usa_campaign_state.h/.cpp` (Continental USA Faction Map & Auto-Sizing Dynamic Legend)**:
   - **Player (`The Project`)**: Starts with **1 territory (`CA` ? California)**.
   - **Neo Paradigm (`Neo Paradigm`)**: Starts with **1 territory (`TX` ? Texas)**.
   - **4-State Scavenger Warlord Clusters**: Remaining 46 states grouped into 12 contiguous regional Scavenger clusters (`Pacific Scavs`, `Rockies Scavs`, `Desert Scavs`, `Badlands Scavs`, `Heartland Scavs`, `Lakes Scavs`, `Ohio Scavs`, `Gulf Scavs`, `Piedmont Scavs`, `Atlantic Scavs`, `New England Scavs`, `Northwoods Scavs`), each with its own muted tactical color.
   - **Dynamic Auto-Sizing 5-Column Map Legend**: Automatically computes exact row height (`rows = (visIdx.size() + 4) / 5`) so every faction swatch, name, and live sector count stays cleanly inside the recessed tactical legend box and shrinks automatically as factions are conquered.
   - **Tactical Retro-Radar Ocean & Flush Neon Borders**: CRT sonar scanlines, animated Pacific & Atlantic sonar rings, continuous radar scan sweep (`SAT-LINK // CONUS GRID 38N 97W`), flush 1px neon-glow state outlines, and unobstructed state initial badges.
2. **`src/design.cpp`, `src/usa_metagame_bridge.cpp` & `src/usa_campaign_state.cpp` (Exclusive Hero General 3D Designer)**:
   - Native 3D Unit Design Screen hooked directly into the Overworld Risk Map with blank-start Dragon Command General build, starting catalog (`Dragon` + `Wheels` + `Heavy Machinegun` / `Medium Cannon` / `Mini-Rocket Pod`), and isolated `SAVED GENERAL VARIANTS` panel.
3. **`src/usa_battle_setup.h/.cpp`, `src/frontend.cpp` & `src/clparse.cpp`**:
   - Custom 2-button Main Menu, Western Scav Syndicate faction registration, Dragon Command General deployment, and structured Scavenger wave pacing.

## Current Milestone
- Verify the cleanly fitted **5-column auto-sizing Dynamic Map Legend**, then build AI faction turns, General Upgrade Tree unlocks, and battlefield Hero General abilities.