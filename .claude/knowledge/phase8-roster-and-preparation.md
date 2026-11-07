# Phase 8a/8d: Roster and Preparation (2026-11-07)

Phase 8 = Roster / Campaign / save. This is the first part: persistent army + pre-battle preparation. Campaign (8b), save JSON (8c), in-battle item pickup (8e) are NOT done.

## Added
- `Roster/Roster.h` (Domain): `Difficulty {Casual, Classic}`, `Roster` (player `BattleUnit`s + gold). `Add` (player only, `IsUnitDataValid`, unique id), `Deploy` (full-heal fresh unit at a position), `TransferItem`, `DiscardItem`, `EquipWeapon` (same rule as `Battle::EquipWeapon`), `ApplyBattleResult(battle, deployedIds, difficulty)`.
- `Battle/Preparation.h` (Application): choose sortie members (limited by start cell count), item changes, `Start()` -> `PreparedBattle {Battle, Deployed}`, `Finish()` back into the roster. Errors: `NoUnitDeployed`, `InvalidSetup`.
- Tests: `RosterTest` (7 cases), `PreparationTest` (7 cases); CTest 19/19.

## Decisions
- Z1: roster members are stored as `BattleUnit` (no separate type) until a second use needs it (rule 5). Map state (position, HasMoved/HasActed) is reset; Hp is set to MaxHp after every battle.
- Z2: a fallen unit = deployed but missing from the finished battle. Classic removes it; Casual keeps its pre-battle state.
- Z3: the preparation UI itself (UMG, .uasset) is not made here; it must be built in the editor on top of `Preparation`.

## Not done
- Shopping in the preparation screen (shop logic exists only in battle).
- Start cells / deploy limit are constructor arguments; chapter JSON (8c) must supply them.
- Equip rule is duplicated in `Roster` and `Battle` (second use now exists; consider extracting a shared helper in a refactor commit).

## Update: Campaign (8b)
- `Battle/Campaign.h` (Application): `Chapter {Name, Setup, StartCells}`, `Campaign(roster, difficulty, chapters)`. `Preparation()` is created lazily per chapter; `Roster()` includes item changes made in the current preparation. `Complete(battle, sortied)` -> `CampaignStep {BattleOngoing, Retry, NextChapter, CampaignCleared}`.
- Z4: on a loss nothing from the battle is applied (retry), but item changes made in the preparation stay. Win: `Preparation::Finish` then the next chapter gets a fresh preparation.
- Z5: `Preparation()` / `CurrentChapter()` / `Complete()` require `!IsFinished()` (assert only, no Result).
- `CampaignTest` (9 cases); CTest 20/20. Chapter JSON and save/load (8c) and in-battle pickups (8e) still not done.
