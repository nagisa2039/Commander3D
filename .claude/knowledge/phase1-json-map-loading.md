# Phase 1: JSON map loading (Infrastructure)

date: 2026-11-07
scope: map JSON text -> MapData (Commander3D module). File I/O is the caller's job. No 3D display.

## Facts

| id | item | file | verified |
|----|------|------|----------|
| J1 | data location decided: `Proj/Content/Data/` (maps in `Data/Maps/*.json`), raw JSON, not .uasset | Proj/Content/Data/Maps/test_map.json | file read by test via FPaths::ProjectContentDir() |
| J2 | JSON schema: `{width,height,terrains:[{moveCost,passable}],cells:[[id,...],...]}`; terrain id = index in `terrains`; cells = rows north->south | Proj/Source/Commander3D/Public/Infrastructure/MapJsonParser.h | Automation Commander3D.Json.* |
| J3 | `FMapJsonParser::Parse(FString) -> Result<MapData, EMapJsonError>`; errors: InvalidJson, InvalidField, RowShapeMismatch (None = unused success marker) | .../Private/Infrastructure/MapJsonParser.cpp | Automation |
| J4 | integers must be whole numbers; terrain id range 0..65535; shape checked only after all fields are well-formed | same | Automation RejectsInvalidInput (10 cases) |
| J5 | game-rule checks (size > 0, id in table) stay in `LoadMap`, not in the parser | MapLoader.h | ParsesValidMap passes parsed data through LoadMap |
| J6 | tests: Commander3D.Json.{ParsesValidMap, RejectsInvalidInput, LoadsSampleMapFile} | Proj/Source/CommanderTests/Private/MapJsonParserTest.cpp | 6/6 Commander3D.* Success; Commander3DEditor build OK (2026-11-07) |
| J7 | Commander3D.Build.cs: Public = Core, CommanderApplication (header exposes MapData); Private = CoreUObject, Engine, InputCore, EnhancedInput, Json, CommanderDomain | Proj/Source/Commander3D/Commander3D.Build.cs | build OK |
| J8 | CommanderTests now depends on Commander3D | Proj/Source/CommanderTests/CommanderTests.Build.cs | build OK |

## Decisions

- D6: Parser reports only 3 error classes; no per-field messages (add when a second consumer needs them, rule 5).
- D7: rows-of-rows `cells` chosen for hand/AI editing; flattened row-major into MapData.Cells.
- D8: Tests use the red->green flow: build failed on missing MapJsonParser.h first.

## UNVERIFIED / open

- Packaging: raw JSON under Content/ may need `+DirectoriesToAlwaysStageAsNonUFS` (DefaultGame.ini) to be staged. Not set; not tested.
- Tests/Core (CTest) not touched this slice; parser is UE-only (FString/FJson*).
- Nothing yet reads the JSON at runtime (no GameMode / Presentation).
- Next candidates: Phase 2 (units / movement), or a Presentation step that draws BattleMap in PL_Test.
