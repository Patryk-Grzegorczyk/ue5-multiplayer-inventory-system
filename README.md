# Grid-Based Multiplayer Inventory System

A simplified portfolio extraction of the grid-based inventory architecture developed for **DeepAnomaly** in Unreal Engine 5 / C++.

The original game system also handles equipment, tools, batteries, clothing, anomalies, quests, crafting, shops, UI and world placement. Those responsibilities are intentionally removed here so the sample focuses on the reusable inventory core.

## Features

- Variable-size inventory items (`FIntPoint Size`)
- Rectangular multi-cell collision detection
- Automatic first-free-position placement
- Explicit item movement to a target cell
- Item rotation with fit validation
- Multiple logical inventory grids
- Server-authoritative state changes
- Replicated inventory grids with `OnRep` callbacks
- Custom `NetSerialize` for inventory data
- UI-independent `OnInventoryChanged` event
- Clear separation between inventory state and presentation code

## Architecture

```text
                Client input / UI
                       |
                       v
              UInventoryComponent
                       |
             +---------+---------+
             |                   |
             v                   v
        Add / Move /        Rotate / Remove
        placement logic          logic
             |                   |
             +---------+---------+
                       |
                       v
                Server authority
                       |
                       v
             FInventoryGrid state
                       |
                       v
                  Replication
                       |
                       v
                    OnRep
                       |
                       v
             OnInventoryChanged
                       |
                       v
                 UI / presentation
```

## Grid placement

Each item occupies a rectangle of cells. The placement algorithm scans the grid from the top-left corner and accepts the first position that:

1. keeps the entire item inside the grid bounds, and
2. does not overlap another item.

The collision test is implemented with axis-aligned rectangle intersection using the item's grid position and size.

### Example

```text
Grid: 6 x 4

+--+--+--+--+--+--+
| A| A|  |  | B| B|
+--+--+--+--+--+--+
| A| A|  |  | B| B|
+--+--+--+--+--+--+
|  |  |  |  |  |  |
+--+--+--+--+--+--+
|  |  |  |  |  |  |
+--+--+--+--+--+--+

A = 2 x 2 item
B = 2 x 2 item
```

## Rotation

Rotation swaps the item's width and height. Before committing the change, the component performs the same bounds and overlap checks used for normal placement. This prevents an item from being rotated into an invalid or occupied space.

## Networking

Inventory mutations are server-authoritative:

```text
Client
  |
  | Server RPC
  v
Server
  |
  | validate request
  v
Inventory state
  |
  | replicated FInventoryGrid
  v
Client OnRep
  |
  v
Presentation update
```

The sample deliberately keeps UI out of the component. `OnInventoryChanged` provides a lightweight integration point for UMG or another presentation layer.

## Why `NetSerialize`?

`FInventoryItem` and `FInventoryGrid` implement custom `NetSerialize` functions and opt into Unreal's `WithNetSerializer` struct trait. This keeps the replicated state representation explicit instead of relying on the default struct serializer.

## Design decisions

### Data-oriented runtime state

The runtime item structure contains only information required by the inventory core:

- identity
- grid size
- position
- category
- rotation state

DeepAnomaly's original structure contains additional references for meshes, widgets, tools, batteries, anomalies, economy and other gameplay systems. Those are intentionally not part of this sample.

### No direct UI dependency

The original component updates `InventoryUserWidget` directly. In this portfolio extraction the core instead broadcasts `OnInventoryChanged`. The UI can subscribe without creating a dependency from the inventory implementation back to a specific widget class.

### Server-side validation

The original game implementation used server RPCs for inventory mutations. This sample keeps that architecture and adds basic parameter validation before applying server-side changes.

## Source relationship

This repository is **not presented as the complete DeepAnomaly inventory implementation**. It is an intentionally simplified extraction of the core architecture used in the game.

The production implementation contains additional game-specific systems and integrations that are outside the scope of this sample.

## Unreal Engine

Designed for Unreal Engine 5 C++ projects.

The generated API macro is `INVENTORYSYSTEM_API`; when integrating these files into another project, replace it with that project's module API macro.

## Portfolio talking points

When discussing this system in an interview, the most relevant topics are:

- How variable-size grid items are represented
- Why rectangle overlap is enough for cell-based placement
- Why placement is deterministic (top-left to bottom-right scan)
- How rotation is validated before modifying state
- Why inventory mutations are server-authoritative in multiplayer
- How replicated state triggers presentation updates
- Why the public sample removes game-specific dependencies from the inventory core
