# Phase 1: Grid and BattleMap (Domain)

date: 2026-11-07
scope: Domain only. No Application loader, no UE-side JSON, no 3D display yet.

## Facts

| id | item | file | verified |
|----|------|------|----------|
| G1 | `Grid::Create(w,h)` -> `std::optional<Grid>`; nullopt if w<=0 or h<=0 | Proj/Source/CommanderDomain/Public/Battle/Grid.h | CTest GridTest pass (MSVC /W4 /WX) |
| G2 | `Grid` API: Width, Height, CellCount, Contains(GridCoord), IndexOf(GridCoord) (row-major, requires Contains) | same | GridTest |
| G3 | `TerrainId` = uint16; `TerrainSpec{MoveCost=1, Passable=true}` | .../Battle/BattleMap.h | BattleMapTest |
| G4 | `BattleMap::Create(Grid, vector<TerrainSpec>, vector<TerrainId>)` -> optional; nullopt if table empty, cells.size()!=CellCount, or any id >= table.size() | same | BattleMapTest |
| G5 | `BattleMap::TerrainAt(GridCoord)` -> `const TerrainSpec*`; nullptr outside grid | same | BattleMapTest |
| G6 | tests registered in Tests/Core/CMakeLists.txt: GridTest, BattleMapTest | Tests/Core/CMakeLists.txt | ctest 4/4 pass |

## Decisions

- D1: failures in Domain return `std::optional`, not `Result`. Reason: Domain must not depend on Application (dependency direction Application -> Domain).
- D2: terrain kinds are data (ids + table), not an enum. Reason: they come from master data (JSON); do not hardcode (rule 5).
- D3: no neighbor/4-adjacency API yet. Add when Phase 2 movement range needs it (second use).
- D4: Grid axes: X grows east, Y grows south, (0,0) = north-west (GridCoord.h).

## Not done / UNVERIFIED

- UE build with these headers: UNVERIFIED (not included from any UE-side file yet).
- GCC build: UNVERIFIED (MSVC only).
- Open user decisions (unanswered): data location (`Proj/Content/Data/` vs `Data/` vs `Proj/Data/`); scope beyond Domain+Application (JSON load, 3D display).
- Next candidate: Application `MapLoader` (validates a JSON-free `MapData`, returns `Result<BattleMap, Error>`).
