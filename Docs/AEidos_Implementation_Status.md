# AEidos Implementation Status

Last reviewed: 2026-08-09

## Current Architecture

- **Simulation:** `WS_SimulationOrchestrator` advances settlement systems with fixed ticks. Economy, population, work, construction, portal, raid, sustenance, item storage, and dungeon runtime are world subsystems.
- **Data:** gameplay definitions are CSV-backed DataTables (`DT_Resource`, `DT_Item`, `DT_Building`, `DT_Work`, `DT_Skill`, `DT_Portalfix`, `DT_Block`, and `DT_BlockInteraction`). `UGIS_DataRegistry` remains the common loader for the originally registered tables.
- **DataAssets:** `DA_EidosDataRegistryConfig` is intentionally retained as the registry configuration asset. `DA_TestDungeon` and `UDungeonSettlementPreset` are intentionally retained as authored dungeon preset snapshots. The old per-block `UWorldBlockDefinition` DataAsset path has been removed; blocks now use `DT_Block` and `DT_BlockInteraction`.
- **Page selection:** `AEidosPlayerController` and `UCameraModeComponent` own the single selected friendly Page. This is not a legacy system: camera follow, first/third-person input, combat turns, inventory, world interaction, portals, and all Page panels use the same selected Page.

## Implemented

### Settlement, Pages, and Construction

- Starter settlement, owned territory chunks, expansion preview, EP cost, and immediate chunk completion.
- Building preview, construction-site work flow, worker assignment, and building list/detail UI.
- Multiple friendly Pages, page cycling with `[` and `]`, camera transfer across streamed settlement/dungeon spaces, and fallback selection when the selected Page dies.
- Page inventory capacity with permitted overweight/overvolume and movement-speed penalty.
- Equipment slots: left/right hand, head, upper body, lower body, and feet.

### Items and Blocks

- Item definitions, stack quantity, quality total, per-Page inventory, and settlement warehouse capacity.
- Return from dungeon converts only configured resource items; normal items and equipment remain with the Page.
- Item panel warehouse/Page lists, Ctrl multi-select, context menu, batch Page-to-warehouse or warehouse-to-Page transfer, placement, equipment, and extensible `Use` actions.
- `InventoryActions` in `DT_Item` controls item-specific context actions. `Move` is contextual; `Drop` becomes available only when `WorldPickupClass` is configured as a `WorldItemBlockActor` class.
- Every interactable terrain/prop/world object follows the block model. `DT_Block` stores shared block state and `DT_BlockInteraction` stores any number of available interactions.
- Equipped right hand is preferred for a block interaction, then left hand. Focus UI presents the prepared left-click action; right-click opens the world interaction menu.
- Block placement consumes a matching inventory item only after valid left-click confirmation and supports a translucent placement preview.

### Dungeons and Combat

- Portal spawn timing/caps, portal interaction, streamed dungeon runtime map, authored settlement-preset capture, enemy spawns, and dungeon core.
- Dungeon core is a hostile combat target. Destruction starts the return window and opens a return portal rather than instantly removing the dungeon.
- Returning Pages leave through the return portal before the dungeon expires.
- Encounter detection, player/enemy turn state, AP consumption, target selection, skill quickbar slots, basic test Slash action, enemy turn behavior, death, and encounter end handling.
- Combat HUD and Page quickbar/editor foundations are implemented.

### UI and Input

- HUD root layers, panel host, close behavior, Buildings / Pages / Dungeons / Items / Research navigation, page detail popup, equipment editor, combat HUD, and world-interaction focus/radial UI.
- First-person UI focus mode and normal third-person UI clicking are supported.
- Custom-depth focused-block outline is enabled through the focus post-process material and world-block stencil state.

## Intentionally Removed

- `UWorldBlockDefinition`: superseded by `DT_Block` and `DT_BlockInteraction`.
- Empty `USelectionSubsystem`: selection is controller/camera-owned.
- Old panel classes and assets not present in the five-panel UI: `Panel_Build`, `Panel_Craft`, `Panel_Recruit`, `Panel_Relations`, and `Panel_Skill`.
- Repeated camera/first-person/click diagnostic logs that produced noisy output during normal play.

## Partial or Placeholder Systems

- Research panel has a host slot but no research progression/content implementation.
- Page skill editor supports quickbar assignment, but only the test combat skill set is available.
- `Use` item actions are exposed to Blueprint through `OnInventoryItemActionRequested`; consumable gameplay effects are not implemented yet.
- Dropping needs a dedicated `WorldPickupClass` derived from `AWorldItemBlockActor` for each droppable item.
- Block data is CSV-driven, but the registry does not yet expose dedicated block-table getters; `AWorldBlockActor` caches table assets locally.
- Dungeon enemy AI, rewards/loot variety, procedural/preset variety, raid resolution, production chains, crafting, research, recruiting, relations, and save/load completeness are unfinished.

## Recommended Next Work

1. Implement actual `Use` effects for consumables and a drop-block blueprint shared by droppable items.
2. Expand block behaviors beyond harvest/pickup: doors, switches, containers, placement rules, and drops.
3. Complete combat action data, target rules, status effects, enemy turns, and post-combat handling.
4. Implement gathering/production/crafting loops, then research and recruitment progression.
5. Add save/load coverage and scenario/raid failure-resolution paths.
