#include <vector>

#include "Battle/BattleMap.h"
#include "TestSupport.h"

using commander::domain::BattleMap;
using commander::domain::Grid;
using commander::domain::TerrainId;
using commander::domain::TerrainSpec;

// Terrain table used by the tests: 0 = plain, 1 = forest, 2 = wall.
static std::vector<TerrainSpec> Table()
{
	return {{1, true}, {2, true}, {0, false}};
}

static BattleMap MakeMap()
{
	// 3x2:  plain forest wall / forest plain plain
	const std::vector<TerrainId> Cells = {0, 1, 2, 1, 0, 0};
	auto Map = BattleMap::Create(*Grid::Create(3, 2), Table(), Cells);
	return *Map;
}

static void CreateRejectsCellCountMismatch()
{
	const Grid G = *Grid::Create(3, 2);
	CHECK(!BattleMap::Create(G, Table(), {0, 0, 0, 0, 0}).has_value());
	CHECK(!BattleMap::Create(G, Table(), {0, 0, 0, 0, 0, 0, 0}).has_value());
}

static void CreateRejectsUnknownTerrainId()
{
	const Grid G = *Grid::Create(3, 2);
	CHECK(!BattleMap::Create(G, Table(), {0, 0, 0, 0, 0, 3}).has_value());
}

static void CreateRejectsEmptyTerrainTable()
{
	const Grid G = *Grid::Create(1, 1);
	CHECK(!BattleMap::Create(G, {}, {0}).has_value());
}

static void TerrainAtReturnsSpecOfCell()
{
	const BattleMap Map = MakeMap();
	CHECK(Map.TerrainAt({0, 0})->MoveCost == 1);
	CHECK(Map.TerrainAt({1, 0})->MoveCost == 2);
	CHECK(Map.TerrainAt({1, 0})->Passable);
	CHECK(!Map.TerrainAt({2, 0})->Passable);
	CHECK(Map.TerrainAt({0, 1})->MoveCost == 2);
}

static void TerrainAtOutsideGridIsNull()
{
	const BattleMap Map = MakeMap();
	CHECK(Map.TerrainAt({3, 0}) == nullptr);
	CHECK(Map.TerrainAt({0, 2}) == nullptr);
	CHECK(Map.TerrainAt({-1, 0}) == nullptr);
}

int main()
{
	CreateRejectsCellCountMismatch();
	CreateRejectsUnknownTerrainId();
	CreateRejectsEmptyTerrainTable();
	TerrainAtReturnsSpecOfCell();
	TerrainAtOutsideGridIsNull();
	return TEST_MAIN_RESULT();
}
