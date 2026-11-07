# Phase 2: units / movement (Domain only)

## Added
- `Battle/BattleUnit.h`: `UnitId`, `Faction` (Player/Enemy), `BattleUnit` (Id, Side, Position, Movement).
- `Battle/MovementRange.h`: `MovementRange` + `CalculateMovementRange(Map, Units, Mover)`. Dijkstra, 4-adjacency, cost = terrain MoveCost of the entered cell.
- `Battle/Battle.h`: aggregate (BattleMap + units). `AddUnit`, `FindUnit`, `UnitAt`, `MoveUnit` -> `std::optional<UnitMoved>` (Event with Path).
- Tests: `Tests/Core/Domain/MovementTest.cpp` (CTest), `CommanderTests/.../BattleMovementWiringTest.cpp` (UE Automation).

## Decisions
- E1: enemy-faction units block; same-faction units can be passed through but not stopped on (FE rule). Reachable != stoppable (`CostTo` vs `CanStopAt`).
- E2: failures return `std::optional` (same as D1). No error reason yet; add when UI needs to explain (rule 5).
- E3: 4-adjacency is inlined in the calculator (the Phase 1 D3 "neighbor API" was not extracted; only one use).
- E4: `MoveUnit` does not limit to one move per phase; that belongs to Phase 3 (turns).

## Verified (2026-11-07)
- CTest 6/6 pass (MSVC /W4 /WX). Domain purity check OK.
- `Commander3DEditor Win64 Development` build succeeded; Automation `Commander3D.Wiring` 4/4 Success.
- Log shows "LogAutomationTest: Error: Condition failed" x19 at startup (before tests); cause unverified, tests unaffected.

## Not done
- Application `MoveUnit` Command / `BattleSession`, unit data in JSON, 3D display, Reachable-range highlight.
- Flying/mounted movement types, terrain-specific unit restrictions.
- CTest on this machine needs vcvars64 loaded first (cmake is not on PATH): see `.claude/temp/runcore.bat` (scratch, untracked).
