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

## Update: campaign / chapter JSON (8c-1)
- **Chapter file** = a battle file under `Data/Chapters/<id>.json` with optional `name` and `startCells` (`FBattleDefinition::Name`, `StartCells`; strict types). Enemies/map/conditions/shops as usual; no player units (they come from the roster).
- **Campaign file** `Data/Campaigns/<name>.json`: `gold`, `roster` (unit objects as in battle `units`; `side`/`x`/`y` optional via `FBattleJsonParser::ParseRosterUnit`), `chapters` (ids, >= 1). `FCampaignLoader::Load` -> `FCampaignDefinition {InitialRoster, Chapters}`. Errors: `ECampaignLoadError`. A chapter without `startCells` is `InvalidChapter`. Difficulty is not in the file (the player picks it).
- `FBattleDataLoader` split into `LoadTables` + `LoadDefinition(dir, tables, subDir, name)`; `Load` behaves as before (refactor, same tests pass).
- `Json` moved to the Commander3D module's Public dependencies (public header exposes `FJsonObject`); `CommanderTests` got `Json` too.
- Samples: `Chapters/test_chapter.json`, `Campaigns/test_campaign.json`. Test `Commander3D.Json.LoadsSampleCampaign`; Automation 15/15.
- Still not done: save/load of progress (8c-2: roster + chapter index + difficulty; weapon points by type name), in-battle pickups (8e).

## Update: save / load (8c-2)
- `Campaign(roster, difficulty, chapters, startIndex = 0)`: resume at a later chapter; an index past the end is clamped to "finished". `CampaignTest` +1 (CTest 20/20).
- `FSaveJsonParser` (`Serialize` / `Parse`, `FSaveData {CampaignName, Difficulty, ChapterIndex, Roster}`): version 1 JSON. Units are saved **self-contained** (equipped weapon, spare weapons/items, promotions, growth, caps written out in full), so editing the master data later does not change a saved army. Only weapon training is keyed by weapon **type name** (this resolves Y6: reordering `types` in the weapon file no longer corrupts saves). Errors: `ESaveJsonError {InvalidJson, UnsupportedVersion, InvalidField, UnknownWeaponType}`. The Roster re-validates every unit (`Roster::Add`).
- `FSaveStorage::Save/Load(DataDir, SaveDir, Slot, ...)`: `<SaveDir>/<Slot>.json`, written via a `.tmp` file + move so a failed write keeps the old save. `Load` re-reads the campaign (chapters) by the saved campaign name and rebuilds the `Campaign`; a chapter index beyond the campaign is `InvalidSave`. Slot/campaign names are file-name safe only.
- Saves are for between battles: unit position/turn state and in-progress battles are not saved; Hp is MaxHp. The save includes item changes made in the current preparation (`Campaign::Roster()`), not the deployment choice.
- Not decided: where `SaveDir` lives (e.g. `FPaths::ProjectSavedDir()/SaveGames`) is up to the caller; no auto-save / slot listing yet.
- Tests: `Commander3D.Save.JsonRoundTrip`, `Commander3D.Save.StorageRoundTrip`; Automation 17/17.
