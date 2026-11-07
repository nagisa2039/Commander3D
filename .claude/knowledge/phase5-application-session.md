# Phase 5 (rest): Application layer — BattleSetup / BattleSession

## Added (`CommanderApplication/Public/Battle/`)
- `BattleSetup.h`: `BattleSetup {MapData, Units, Conditions}`, `StartBattle(setup)` -> `Result<Battle, BattleSetupError>`. Errors (checked in this order): `InvalidMap`, `InvalidUnit`, `NoPlayerUnit`, `NoEnemyUnit`, `NoBoss`.
- `BattleSession.h`: Commands `MoveUnitCommand`, `WaitUnitCommand`, `AttackUnitCommand`, `EndPhaseCommand` (`BattleCommand` variant); `BattleEvent` = variant of Domain events; `BattleSession::Execute(cmd)` -> `Result<BattleEvent, CommandError {BattleEnded, Rejected}>`.
- Tests: `Tests/Core/Application/BattleSetupTest.cpp` (6), `BattleSessionTest.cpp` (4); UE `BattleSessionWiringTest.cpp`.

## Decisions
- S1: only two command errors. The Domain returns no reason, and re-deriving reasons in Application would duplicate rules (rule 4). Add reasons to the Domain first if the UI needs them.
- S2: `StartBattle` validates what Phase 5 knowledge flagged: units must be placeable, both sides present, boss present when `DefeatBoss` is on. This closes the "enemy-less map is won immediately" hazard for maps built via `StartBattle`.
- S3: `BattleSession` holds `IRandomSource&` (must outlive the session) and a copy of the `Battle`.
- S4: `InvalidMap` does not carry the `MapLoadError` detail (not needed yet).

## Verified (2026-11-07)
- CTest 11/11, purity check OK, `Commander3DEditor` build OK, Automation `Commander3D.Wiring` 5/5 Success.

## Not done
- JSON for units/weapons/conditions and Infrastructure parsing (only maps are parsed today) — so there is still no data-driven "clear one map" path.
- Enemy AI (Phase 6) — it should issue these same Commands. Presentation / UI.
- Production `IRandomSource` implementation (only test fakes exist).
