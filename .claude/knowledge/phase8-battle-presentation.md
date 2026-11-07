# Phase 8g: battle presentation foundation (2026-11-07)

C++ half of the battle display. Needs no art asset (engine primitive shapes) and no `.uasset`. Layout / input / dressing are for the editor.

## Added (module Commander3D, `Presentation/`)
- `UCommanderBattleController` (UObject, no World needed): `BeginBattle(subsystem)`, queries (`GetUnits`, `GetChests`, `GetShopCells`, `GetMapSize`, `GetCellMoveCost`, `GetTurn`, `IsPlayerPhase`, `GetGold`, `GetResult`, `GetCampaignStep`, `GetMovableCells(unit)`, `GetAttackableTargets(unit)`), commands (`MoveUnit`, `WaitUnit`, `AttackUnit`, `OpenChest`, `EndPlayerPhase`) and delegates `OnLog(text)`, `OnChanged`, `OnFinished(result, step)`.
  - Commands only run in the player phase and are forwarded to `BattleSession`; `EndPlayerPhase` ends the phase and plays the enemy phase with `RunEnemyPhase`. When the outcome is decided, `Campaign::Complete` is called through the subsystem (`CompleteBattle`) exactly once. A finished controller can `BeginBattle` again (retry / next chapter).
  - `GetMovableCells` / `GetAttackableTargets` only call Domain queries (`CalculateMovementRange`, `WeaponReaches`); no rule is written in the Presentation.
- `ACommanderBattleBoard` (AActor): draws the battle with `/Engine/BasicShapes` Cube / Cylinder and `BasicShapeMaterial` (parameter "Color"; unverified in a rendered session): a tile per cell (walls stand up), blue / red / dark-red cylinders for player / enemy / boss (dimmed after acting, height follows Hp), cubes for chests and shops. `StartBattle()` gets the campaign subsystem from the game instance; `CellToWorld`, `WorldToCell`, `HighlightCells` help input handling. Redraws on `OnChanged`.
- Tests: `Commander3D.Presentation.BattleController` (whole battle incl. both outcomes of the random result; run 3 times, green each time); Automation 19/19. The actor itself is only built, not run (needs a World and rendering): unverified.

## Not done
- Animation (movement, strikes), health bars, unit names / portraits, a cursor and input mapping (EnhancedInput assets are `.uasset`), camera. Item use / staff / promotion / buy-sell in battle have no controller commands yet (the Domain and session support them; events are logged as "Something happened").
- Widget Blueprints for the preparation and campaign screens (see `phase8-presentation-subsystem.md`).
