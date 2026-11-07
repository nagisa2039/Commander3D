# Phase 5: victory conditions (Domain only; map start/end not done)

## Update: boss defeat + turn limit (2026-11-07)
- `VictoryConditions {RoutEnemies=true, DefeatBoss=false, TurnLimit=0}`, passed to `Battle(Map, Conditions)`; `BattleUnit::IsBoss`.
- `EvaluateOutcome(units, conditions, turn)` precedence: no player units -> lost; rout met -> won; boss rule met (no enemy-side `IsBoss` left) -> won; `TurnLimit > 0 && turn > TurnLimit` -> lost; else ongoing. A win on the last allowed turn is a win.
- Outcome is now evaluated after each attack AND in `EndPhase` (after the turn advances). `PhaseChanged::Outcome` added. With limit N, the `EndPhase` that would start turn N+1 returns PlayerLost (Turn = N+1 in the event).
- Consequence: a battle with no enemy units ends as PlayerWon at the first `EndPhase` (rout rule). Real maps must have enemies; a map using `DefeatBoss` must have a boss (otherwise it is won immediately). The loader should validate both.
- A wiring test failed once because its fixture had no enemies (test data, not code); fixed in the test.
- Tests: `VictoryTest.cpp` now 13 cases. CTest 9/9, `Commander3DEditor` build OK, Automation `Commander3D.Wiring` 4/4.
- V1/V2 below are the original (pre-update) decisions; V1's "no second condition" no longer applies; V2 is superseded by the EndPhase evaluation above.

## Added
- `Battle/VictoryCondition.h`: `BattleOutcome {Ongoing, PlayerWon, PlayerLost}`, `EvaluateOutcome(units)`.
- `Battle::Outcome()`; `AttackResolved::Outcome`. Outcome is evaluated after each attack.
- Once decided: `MoveUnit` / `WaitUnit` / `AttackUnit` return nullopt; `EndPhase` changes nothing and returns the current phase.
- Tests: `Tests/Core/Domain/VictoryTest.cpp` (6 cases); wiring test extended.

## Decisions
- V1: only rout (annihilation). No player units -> lost (checked first); else no enemies -> won. Seize / boss-defeat / turn-limit are not added until a second concrete condition is needed (rule 5).
- V2: outcome is evaluated only after an attack. A battle that starts with no enemies stays Ongoing until something is evaluated; map loading (not yet built) should reject or handle such maps.
- V3: `EndPhase` keeps its non-optional return; after the battle ends it is a no-op.

## Verified (2026-11-07)
- CTest 9/9, purity check OK, `Commander3DEditor` build succeeded, Automation `Commander3D.Wiring` 4/4 Success.

## Not done (needed for the "clear one map" vertical slice)
- Building a `Battle` from map data + unit/weapon data (JSON, Infrastructure) = "map start".
- Application layer (`BattleSession`, Commands, `Result`), enemy AI (Phase 6; without it enemy phases must be skipped/ended manually), Presentation.
