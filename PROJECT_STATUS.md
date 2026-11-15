# Warzone 2100: USA Reclamation Initiative (Standalone RTW Fork)

## Repository & Architecture
- **Repo:** https://github.com
- **Local Path:** C:\Games\warzone2100_source (Build output: build\src\warzone2100.exe)

## Completed Milestones
1. **`src/usa_campaign_state.h/.cpp` & `src/usa_overworld_ui.cpp` (Pre-Map Player Faction Setup Screen)**:
   - Opens **Strategic Command Faction Initialization** right before the USA Risk Map launches (and whenever starting a New Campaign).
   - **Custom Faction Name**: Direct keyboard typing + quick-select presets, reflected live across the Map Legend, Sector Panel, Header, and PHYSFS save file (`PLAYER_FACTION`).
   - **8 Exclusive Player Colors**: Reserved palette (`Cobalt Blue`, `Emerald Cyan`, `Crimson Red`, `Royal Violet`, `Arctic Gold`, `Titanium Silver`, `Deep Indigo`, `Teal Horizon`) guaranteed never to be used by Neo Paradigm or any of the 12 Scavenger Warlord clusters.
2. **`src/usa_overworld_ui.cpp` & `src/usa_campaign_state.h/.cpp` (Continental USA Faction Map & Auto-Sizing Dynamic Legend)**:
   - **Player (`The Project` / Custom Name)**: Starts with **1 territory (`CA` ? California)**.
   - **Neo Paradigm (`Neo Paradigm`)**: Starts with **1 territory (`TX` ? Texas)**.
   - **4-State Scavenger Warlord Clusters**: Remaining 46 states grouped into 12 contiguous regional Scavenger clusters (`Pacific Scavs`, `Rockies Scavs`, `Desert Scavs`, `Badlands Scavs`, `Heartland Scavs`, `Lakes Scavs`, `Ohio Scavs`, `Gulf Scavs`, `Piedmont Scavs`, `Atlantic Scavs`, `New England Scavs`, `Northwoods Scavs`), each with its own muted tactical color.
   - **Dynamic Auto-Sizing 5-Column Map Legend**: Automatically adjusts row count and height so every faction swatch, name, and live sector count stays cleanly inside the recessed tactical legend box and updates live as factions expand or are eliminated.
   - **Tactical Retro-Radar Ocean & Flush Neon Borders**: CRT sonar scanlines, animated Pacific & Atlantic sonar rings, continuous radar scan sweep (`SAT-LINK // CONUS GRID 38N 97W`), flush 1px neon-glow state outlines, and unobstructed state initial badges.
3. **`src/design.cpp`, `src/usa_metagame_bridge.cpp` & `src/usa_campaign_state.cpp` (Exclusive Hero General 3D Designer)**:
   - Native 3D Unit Design Screen hooked directly into the Overworld Risk Map with blank-start Dragon Command General build (`IDDES_TEMPLSTART` auto-active), starting catalog (`Dragon` + `Wheels` + `Heavy Machinegun` / `Medium Cannon` / `Mini-Rocket Pod`), and isolated `SAVED GENERAL VARIANTS` panel.
4. **`src/usa_battle_setup.h/.cpp`, `src/frontend.cpp` & `src/clparse.cpp`**:
   - Custom 2-button Main Menu, Western Scav Syndicate faction registration, Dragon Command General deployment, and structured Scavenger wave pacing.

## Next Planned Milestones (Future Improvements)
- **General Upgrade Tree Progression & Rank Unlocks**: Wire the 6 General upgrade levels and combat rank milestones to progressively unlock higher-tier propulsions (`HalfTrack`, `tracked01`, `hover01`) and advanced twin-turret weapons inside the General Design Bay.
- **Multi-Faction Overworld AI & Campaign Turn Loop**: Implement AI expansion and territory battles across the 48 states for Neo Paradigm and the 12 Scavenger Warlord clusters.
- **Battlefield Hero General Abilities & HUD**: Add active command abilities, veterancy promotion tracking across territories, and tactical rally controls for the Dragon Command General in live battles.