# Warzone 2100: USA Reclamation Initiative (Standalone RTW Fork)

## Repository & Architecture
- **Repo:** https://github.com
- **Local Path:** C:\Games\warzone2100_source (Build output: build\src\warzone2100.exe)

## Completed Milestones
1. **`src/usa_campaign_state.h/.cpp`**:
   - Strict planar shared-vertex 48-state continental USA Risk map with full PHYSFS persistence.
   - `UsaGeneralState` Dragon Command General hero unit state starting unbuilt on a fresh campaign so the player constructs their first General from scratch.
   - Dedicated `UsaGeneralVariant` saved variant list (`GENERAL_ACTIVE` & `GENERAL_VARIANT` serialization), strictly isolated from normal `camp.squad` unit blueprints, plus the 6-node General upgrade tree.
2. **`src/usa_battle_setup.h/.cpp`**:
   - Registered **Western Scav Syndicate** as a full native faction (`usaUnlockScavFactionForPlayer` + `usaGetScavFactionTemplates`).
   - Automatic battle deployment of Player 0's constructed **Dragon Command General** (`Body14SUP` + `wheeled01` + built-in `CommandBrain01` + **2 full weapon slots** + HP/Speed/Rank upgrade multipliers).
3. **`src/design.cpp`, `src/usa_metagame_bridge.cpp` & `src/usa_overworld_ui.cpp`**:
   - Hooked Warzone 2100's **actual native 3D Unit Design Screen (`src\design.cpp`)** directly into the Overworld Risk Map (`DESIGN` tab & forked `GENERAL` tab) with **zero Skirmish loading** and flicker-free rendering.
   - **Exclusive Hero General Designer**:
     - Auto-triggers `IDDES_TEMPLSTART` ("New Design") on entry so the turntable starts blank and the player can assemble and save their first General immediately via **Store Design (`IDDES_STOREBUTTON`)**.
     - Starting base locked strictly to **Dragon (`Body14SUP`)** only (unavailable as a buildable unit for any faction/player), **Wheels (`wheeled01`)** only, and **Heavy Machinegun (`MG3Mk1`)**, **Medium Cannon (`Cannon2A-TMk1`)**, and **Mini-Rocket Pod (`Rocket-Pod`)** only.
     - Dedicated **SAVED GENERAL VARIANTS** panel directly on the **`GENERAL`** tab to inspect and click-to-equip saved General variants.
4. **`src/frontend.cpp` & `src/clparse.cpp`**:
   - Custom 2-button Main Menu (`Global Reclamation Campaign` + `Options`) entering the Risk Map directly with zero external launchers.
5. **`src/usa_metagame_bridge.cpp`**:
   - Structured Scavenger wave pacing (`Wave 1: 65 cap`, `Wave 2: 85 cap`, `Wave 3: 105 cap` up to `140`), **18-tile** close-perimeter forward rally staging for **150s (2m 30s)**, and **600s (10m)** sustained Mass Assault windows.

## Next Planned Milestones (Future Improvements)
- **General Upgrade Tree Progression & Rank Unlocks**: Wire the 6 General upgrade levels and combat rank milestones to progressively unlock higher-tier propulsions (`HalfTrack`, `Tracked01`, `hover01`) and advanced twin-turret weapons inside the General Design Bay.
- **Battlefield Hero General Abilities & HUD**: Add active command abilities, veterancy promotion tracking across territories, and tactical rally controls for the Dragon Command General in live battles.
- **Multi-Faction Overworld AI & Campaign Victory**: Implement AI turn expansion across the 48 states for New Paradigm, The Collective, NEXUS Remnant, and Western Scav Syndicate.