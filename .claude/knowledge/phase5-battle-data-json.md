# Phase 5 (rest): battle data in JSON (Infrastructure) + production RNG

## Added
- Domain: `Battle::AddUnit` also rejects inconsistent weapons (negative range, MinRange > MaxRange, Hit outside 0..100). Test: `CombatTest::AddUnitRejectsInconsistentWeapons`.
- Application: `Battle/StdRandomSource.h` (`StdRandomSource(seed)`, mt19937, 0..99). Test: `Tests/Core/Application/StdRandomSourceTest.cpp`.
- Infrastructure (`Commander3D` module, `Infrastructure/`):
  - `JsonReadHelpers.h` (Private): `JsonRead::ToInt / ReadInt32Field / ReadIntFieldInRange / ReadOptionalBoolField / ReadStringField`. Extracted from `MapJsonParser.cpp` (behaviour unchanged) because the weapon/battle parsers are the second and third users.
  - `FWeaponJsonParser::Parse(text) -> Result<FWeaponTable, EWeaponJsonError>`.
  - `FBattleJsonParser::Parse(text, weapons) -> Result<FBattleDefinition, EBattleJsonError>` (None / InvalidJson / InvalidField / UnknownWeapon).
  - `FBattleDataLoader::Load(dataDir, battleName) -> Result<BattleSetup, EBattleLoadError>`; reads `Weapons.json`, `Battles/<name>.json`, `Maps/<map>.json`.
- Data: `Proj/Content/Data/Weapons.json`, `Data/Battles/test_battle.json` (on `test_map`: 2 player + 2 enemy incl. a boss, defeatBoss + turnLimit 20).
- Tests: `CommanderTests/Private/BattleDataJsonTest.cpp` — `Commander3D.Json.{ParsesWeapons, RejectsInvalidWeapons, ParsesBattle, RejectsInvalidBattle, LoadsSampleBattle}`.

## Formats
- Weapons: `{"weapons":[{"id","might","hit","minRange","maxRange"}]}`; duplicate ids rejected.
- Battle: `{"map","conditions"?{routEnemies,defeatBoss,turnLimit},"units":[{id,side:"player"|"enemy",x,y,movement,hp,strength,skill,speed,defense,weapon?,boss?}]}`. `hp` sets Hp and MaxHp; no weapon = unarmed (MaxRange 0).

## Decisions
- B1: no unit templates / classes in JSON — stats are inline per placement until a second map reuses them (rule 5; classes are Phase 7).
- B2: weapon sanity is checked in the Domain (`AddUnit`), not the parser, so rules stay in one place; a weapon never used by a unit is not validated.
- B3: battle and map names are used as file names, so only `[A-Za-z0-9_-]` is accepted (`InvalidName`; for the map name inside the battle file -> `InvalidBattle`). Blocks `../` traversal.
- B4: the loader takes `DataDir` as an argument (tests pass `ProjectContentDir()/Data`); it does not call `StartBattle` — the caller does, so the two error spaces stay separate.
- B5: `"boss": 1` must be rejected: UE's `FJsonValue::TryGetBool` accepts numbers, so the helper checks `Type == EJson::Boolean`. Found by the test.
- B6 (unverified side effect): `MapJsonParser` still uses `TryGetBoolField` for `"passable"`, which probably accepts numbers the same way. Not changed or tested.

## Verified (2026-11-07)
- CTest 12/12, purity check OK, `Commander3DEditor` build OK, Automation `Commander3D.*` 13/13 Success.

## Not done
- Packaging/staging of `Content/Data/*.json` (still unverified).
- Nothing at runtime calls `FBattleDataLoader` yet (no GameMode / Presentation), so the "clear one map" path exists as data + Domain + Application but cannot be played.
- Enemy AI (Phase 6) and Presentation.
