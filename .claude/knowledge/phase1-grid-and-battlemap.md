# Phase 1: Grid and BattleMap (Domain)

date: 2026-11-07
scope: Domain + Application (MapLoader). No UE-side JSON, no 3D display yet.

## Facts

| id | item | file | verified |
|----|------|------|----------|
| G1 | `Grid::Create(w,h)` -> `std::optional<Grid>`; nullopt if w<=0 or h<=0 | Proj/Source/CommanderDomain/Public/Battle/Grid.h | CTest GridTest pass (MSVC /W4 /WX) |
| G2 | `Grid` API: Width, Height, CellCount, Contains(GridCoord), IndexOf(GridCoord) (row-major, requires Contains) | same | GridTest |
| G3 | `TerrainId` = uint16; `TerrainSpec{MoveCost=1, Passable=true}` | .../Battle/BattleMap.h | BattleMapTest |
| G4 | `BattleMap::Create(Grid, vector<TerrainSpec>, vector<TerrainId>)` -> optional; nullopt if table empty, cells.size()!=CellCount, or any id >= table.size() | same | BattleMapTest |
| G5 | `BattleMap::TerrainAt(GridCoord)` -> `const TerrainSpec*`; nullptr outside grid | same | BattleMapTest |
| G6 | tests registered in Tests/Core/CMakeLists.txt: GridTest, BattleMapTest | Tests/Core/CMakeLists.txt | ctest 4/4 pass |
| G7 | UE wiring test Commander3D.Wiring.BattleMapIsUsable includes BattleMap.h (first UE-side include of Grid/BattleMap) | Proj/Source/CommanderTests/Private/BattleMapWiringTest.cpp | Commander3DEditor Win64 Development build OK; UnrealEditor-Cmd Automation RunTests Commander3D.Wiring: 2/2 Success (2026-11-07) |
| G8 | `MapData{Width,Height,Terrains,Cells}`, `MapLoadError{InvalidSize,EmptyTerrainTable,CellCountMismatch,UnknownTerrainId}`, `LoadMap(MapData) -> Result<BattleMap,MapLoadError>`; checks run in enum order | Proj/Source/CommanderApplication/Public/Map/MapLoader.h | CTest MapLoaderTest pass (5/5 total) |
| G9 | UE wiring test Commander3D.Wiring.MapLoaderIsUsable | Proj/Source/CommanderTests/Private/MapLoaderWiringTest.cpp | UE build OK; Automation Commander3D.Wiring 3/3 Success (2026-11-07) |

## Decisions

- D1: failures in Domain return `std::optional`, not `Result`. Reason: Domain must not depend on Application (dependency direction Application -> Domain).
- D2: terrain kinds are data (ids + table), not an enum. Reason: they come from master data (JSON); do not hardcode (rule 5).
- D3: no neighbor/4-adjacency API yet. Add when Phase 2 movement range needs it (second use).
- D5: MapData is format-agnostic (no JSON in Application). JSON parsing belongs to Infrastructure (Commander3D, FJsonObject); not implemented.
- D4: Grid axes: X grows east, Y grows south, (0,0) = north-west (GridCoord.h).

## Not done / UNVERIFIED

- UE build with these headers: verified via G7 (compiled by CommanderTests, run with -nullrhi).
- GCC build: out of scope (user decision 2026-11-07: UE/MSVC toolchain verification is sufficient).
- Open user decisions (unanswered): data location (`Proj/Content/Data/` vs `Data/` vs `Proj/Data/`); scope beyond Domain+Application (JSON load, 3D display).
- Next candidate: Infrastructure JSON -> MapData (needs data location decision) or Phase 2 units/movement.
