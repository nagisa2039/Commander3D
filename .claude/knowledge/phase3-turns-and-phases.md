# Phase 3: turns / phases (Domain only)

## Added
- `BattleUnit`: `HasMoved`, `HasActed`.
- `Battle`: `CurrentFaction()` (starts Player), `TurnNumber()` (starts 1), `WaitUnit` -> `optional<UnitWaited>`, `EndPhase` -> `PhaseChanged {Side, Turn}`.
- `MoveUnit` now requires: unit of the current faction, not HasActed, not HasMoved. Sets HasMoved.
- Tests: `Tests/Core/Domain/TurnTest.cpp` (7 cases); wiring test extended (`BattleMovementIsUsable`).

## Decisions
- T1: one move per phase, then Wait (FE: no move undo). Attack (Phase 4) will also set HasActed.
- T2: `EndPhase` is explicit (manual). No auto end when all units acted; the caller/UI decides. Add when needed.
- T3: turn number increments when control returns to Player (Enemy -> Player). Only 2 factions are supported (Player/Enemy); a third faction (ally NPC) would need a phase order list, not done (rule 5).
- T4: Event member named `Side`, not `Faction` (name would clash with the type).

## Verified (2026-11-07)
- CTest 7/7, purity check OK, `Commander3DEditor` build succeeded, Automation `Commander3D.Wiring` 4/4 Success.

## Not done
- Application Commands (`MoveUnit`, `WaitUnit`, `EndPhase`) / `BattleSession`; victory check; units on dead/removed (Phase 4+).
