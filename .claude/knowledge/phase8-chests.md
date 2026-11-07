# Phase 8e: treasure chests (2026-11-07)

In-battle item pickup. A chest sits on a map cell; a player unit that stands on it opens it with `OpenChest`.

## Added
- `Battle/Chest.h` (Domain): `Chest {Position, std::optional<ItemSlot> Item, Gold}`: exactly one of item / gold.
- `Battle::AddChest` (cell passable and in the grid, one chest per cell, exactly one valid content, Gold >= 0), `Battle::Chests()`, `Battle::OpenChest(unit)` -> `ChestOpened {Unit, Position, Item, Gold}`.
- Application: `OpenChestCommand`, `ChestOpened` in `BattleEvent`; `BattleSetup::Chests`; new error `BattleSetupError::InvalidChest` (checked after shops, before NoPlayerUnit).
- Infrastructure: battle / chapter JSON `"chests": [{"x","y","item": id} | {"x","y","gold": n}]` (item ids as in `items`: weapon id = fresh weapon). Errors: `InvalidField` (not exactly one of item/gold, wrong types), `UnknownItem`. Sample: `Chapters/test_chapter.json` has an item chest and a gold chest.
- Tests: `ChestTest` (5 cases), `BattleSessionTest` +1, `BattleSetupTest` +1 (CTest 21/21); `Commander3D.Json.*` extended; Automation 17/17.

## Rules
- Opening ends the unit's action (HasMoved and HasActed), like `UseItem`; the unit may move onto the chest first. Player units only; enemies never open chests.
- An item chest is refused when the pack is full (the chest stays); a gold chest has no such limit. Gold goes into the army gold and is carried into the roster by `ApplyBattleResult`; the item goes into the opener's pack and is carried over with the unit.

## Decisions
- E1: a chest is its own concept (not a shop variant): it is consumed, has one content, and costs the action.
- E2: no keys / locked chests, no "visit" tiles (villages) and no enemy item drops yet; add them when a second use appears.
- E3: the opening unit that dies later in the same battle takes the item with it (classic rule); a Casual revive restores its pre-battle state, so a chest item taken by a fallen unit is lost.

## Not done
- Enemy AI does not go for chests. The Presentation (showing chests, the open animation) is not made.
