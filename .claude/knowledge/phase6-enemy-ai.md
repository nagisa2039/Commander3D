# Phase 6: enemy AI (Application)

## Added
- `CommanderApplication/Public/Battle/EnemyAi.h`:
  - `DecideNext(const Battle&) -> optional<BattleCommand>`: next Command for the enemy side; nullopt outside the enemy phase or after the battle ended; `EndPhaseCommand` when every enemy unit has acted.
  - `RunEnemyPhase(BattleSession&) -> vector<BattleEvent>`: executes DecideNext until EndPhase, a rejected Command, or battle end (then no EndPhase). Loop is capped at units*3+1.
- Tests: `Tests/Core/Application/EnemyAiTest.cpp` (11 cases); UE wiring test extended (enemy phase through the session).

## Behaviour (per enemy unit, list order)
1. Best attack over reachable stoppable cells x player targets in weapon range. Order: kill > more damage > cheaper move > lower target id. Move first (MoveUnitCommand) if the cell differs from the current one, then AttackUnitCommand on the next call.
2. Otherwise approach: the reachable cell with the smallest Manhattan distance to any player unit (ties: cheaper cost). Moves only if strictly closer.
3. Otherwise Wait.

## Decisions
- A1: the AI lives in Application, not Domain, because it emits Application Commands; it only reads `Battle` and acts through `BattleSession` (same path as the player, per design doc).
- A2: stateless — decisions are derived from `HasMoved` / `HasActed`, so no AI memory is needed between calls.
- A3: scoring ignores hit chance, counter-damage taken and terrain. Deliberately simple; add only with a concrete behaviour requirement.
- A4: reuses Domain's `CalculateMovementRange`, `CalculateDamage`, `WeaponReaches` (no duplicate rules in Application).

## Update: approach uses path distance (2026-11-07)
- `ApproachCell` now picks the reachable cell with the smallest terrain path cost (movement-cost weighted) to any player unit, instead of Manhattan distance. Ties: cheaper move; then first in row-major cell order.
- Distance fields: `PlayerPathCostFields` = `CalculateMovementRange(map, no units, playerCopy with Movement 1<<20)` per player unit. Units do not block the field, so a crowd cannot make a target look unreachable; actual stepping still respects blocking via the candidate cells.
- Player units walled off from every cell are ignored (unit waits if no player is reachable by path).
- Tests added: `ApproachFollowsThePathAroundAWall`, `ApproachPrefersTheShorterOfTwoRoutes`, `ApproachChoosesTheReachablePlayerWhenAnotherIsWalledOff`, `WaitsWhenNoPlayerUnitIsReachableByPath` (EnemyAiTest now 15 cases). The first and third failed (red) under the Manhattan version before the change.
- Cost: one `CalculateMovementRange` per player unit per decision (no caching); fine for "a few dozen units", not measured.
- Because only terrain is considered, an enemy may still be blocked for a turn by units in a corridor (allies are passed through but not stopped on; player units block). Not tested.
- Superseded: the Manhattan-distance limitation below no longer applies.

## Known limitations
- (superseded, see update) Approach uses Manhattan distance, not path distance: a unit can stall behind a wall (U-shaped obstacle). Not tested.
- Only the Player faction is targeted; no retreat, no healing, no formation, no aggro range (enemies always advance).
- Order of acting = Units() list order (not sorted by anything).

## Verified (2026-11-07)
- CTest 13/13, purity check OK, `Commander3DEditor` build OK, Automation `Commander3D.*` 13/13 Success.

## Not done
- Nothing runs `RunEnemyPhase` at runtime yet (no GameMode / Presentation).
- Different AI behaviours per unit (data-driven AI type), if needed later.
