#include <vector>

#include "Battle/Battle.h"
#include "Battle/MovementRange.h"
#include "TestSupport.h"

using namespace commander::domain;

// Terrain table: 0 = plain (cost 1), 1 = forest (cost 2), 2 = wall (impassable).
static std::vector<TerrainSpec> Table()
{
	return {{1, true}, {2, true}, {0, false}};
}

// 5x3:
//   . . . . .
//   . F W . .
//   . . . . .
static BattleMap MakeMap()
{
	const std::vector<TerrainId> Cells = {0, 0, 0, 0, 0, 0, 1, 2, 0, 0, 0, 0, 0, 0, 0};
	return *BattleMap::Create(*Grid::Create(5, 3), Table(), Cells);
}

static BattleUnit MakeUnit(std::uint32_t Id, Faction F, GridCoord Pos, std::int32_t Move)
{
	return BattleUnit{UnitId{Id}, F, Pos, Move};
}

static void RangeOnOpenPlainUsesManhattanDistance()
{
	const BattleMap Map = MakeMap();
	const BattleUnit U = MakeUnit(1, Faction::Player, {0, 0}, 2);
	const MovementRange R = CalculateMovementRange(Map, {U}, U);
	CHECK(R.CanStopAt({0, 0}));
	CHECK(R.CanStopAt({2, 0}));
	CHECK(R.CostTo({0, 2}) == 2);
	CHECK(R.CostTo({1, 0}) == 1);
	CHECK(!R.CanStopAt({1, 1}));  // forest: cheapest route costs 1 + 2 = 3 > 2
}

static void ForestCostsMore()
{
	const BattleMap Map = MakeMap();
	const BattleUnit U = MakeUnit(1, Faction::Player, {0, 1}, 2);
	const MovementRange R = CalculateMovementRange(Map, {U}, U);
	CHECK(R.CostTo({1, 1}) == 2);
	CHECK(!R.CanStopAt({2, 1}));  // wall
	CHECK(R.CostTo({2, 1}) < 0);
}

static void RangeIsLimitedByMovement()
{
	const BattleMap Map = MakeMap();
	const BattleUnit U = MakeUnit(1, Faction::Player, {0, 0}, 3);
	const MovementRange R = CalculateMovementRange(Map, {U}, U);
	CHECK(R.CostTo({3, 0}) == 3);
	CHECK(!R.CanStopAt({4, 0}));
	CHECK(R.CostTo({4, 0}) < 0);
}

static void WallsAreRoutedAround()
{
	const BattleMap Map = MakeMap();
	const BattleUnit U = MakeUnit(1, Faction::Player, {1, 0}, 5);
	const MovementRange R = CalculateMovementRange(Map, {U}, U);
	// (1,0) -> (2,0) -> (3,0) -> (3,1): wall at (2,1) is skipped.
	CHECK(R.CostTo({3, 1}) == 3);
	CHECK(R.CostTo({2, 2}) == 4);  // (1,0)->forest(1,1)=2 ->(1,2)=1 ->(2,2)=1; going around on plains costs 5
}

static void EnemyBlocksButAllyIsPassedThrough()
{
	const BattleMap Map = MakeMap();
	const BattleUnit Mover = MakeUnit(1, Faction::Player, {0, 0}, 4);
	const BattleUnit Ally = MakeUnit(2, Faction::Player, {1, 0}, 1);
	const BattleUnit Foe = MakeUnit(3, Faction::Enemy, {0, 1}, 1);
	const std::vector<BattleUnit> Units = {Mover, Ally, Foe};
	const MovementRange R = CalculateMovementRange(Map, Units, Mover);
	CHECK(R.CostTo({1, 0}) == 1);       // passable...
	CHECK(!R.CanStopAt({1, 0}));         // ...but cannot stop on an ally
	CHECK(R.CanStopAt({2, 0}));          // and can go beyond it
	CHECK(R.CostTo({0, 1}) < 0);         // enemy tile is not enterable
	CHECK(!R.CanStopAt({0, 1}));
}

static void PathToListsCellsFromStartToDestination()
{
	const BattleMap Map = MakeMap();
	const BattleUnit U = MakeUnit(1, Faction::Player, {0, 0}, 3);
	const MovementRange R = CalculateMovementRange(Map, {U}, U);
	const std::vector<GridCoord> Path = R.PathTo({2, 0});
	CHECK(Path.size() == 3);
	CHECK(Path.front() == (GridCoord{0, 0}));
	CHECK(Path.back() == (GridCoord{2, 0}));
	CHECK(R.PathTo({4, 2}).empty());  // out of range
}

static void BattleAddUnitValidates()
{
	Battle B(MakeMap());
	CHECK(B.AddUnit(MakeUnit(1, Faction::Player, {0, 0}, 3)));
	CHECK(!B.AddUnit(MakeUnit(1, Faction::Player, {1, 0}, 3)));   // duplicate id
	CHECK(!B.AddUnit(MakeUnit(2, Faction::Enemy, {0, 0}, 3)));    // occupied
	CHECK(!B.AddUnit(MakeUnit(3, Faction::Enemy, {2, 1}, 3)));    // wall
	CHECK(!B.AddUnit(MakeUnit(4, Faction::Enemy, {5, 0}, 3)));    // outside grid
	CHECK(!B.AddUnit(MakeUnit(5, Faction::Enemy, {1, 0}, -1)));   // negative movement
	CHECK(B.Units().size() == 1);
}

static void BattleMoveUnitReturnsEventAndUpdatesPosition()
{
	Battle B(MakeMap());
	B.AddUnit(MakeUnit(1, Faction::Player, {0, 0}, 3));
	const auto Event = B.MoveUnit(UnitId{1}, {2, 0});
	CHECK(Event.has_value());
	CHECK(Event->Unit == UnitId{1});
	CHECK(Event->Path.front() == (GridCoord{0, 0}));
	CHECK(Event->Path.back() == (GridCoord{2, 0}));
	CHECK(B.FindUnit(UnitId{1})->Position == (GridCoord{2, 0}));
	CHECK(B.UnitAt({0, 0}) == nullptr);
	CHECK(B.UnitAt({2, 0}) != nullptr);
}

static void BattleMoveUnitRejectsInvalidMoves()
{
	Battle B(MakeMap());
	B.AddUnit(MakeUnit(1, Faction::Player, {0, 0}, 2));
	B.AddUnit(MakeUnit(2, Faction::Player, {1, 0}, 2));
	CHECK(!B.MoveUnit(UnitId{9}, {1, 1}).has_value());   // unknown unit
	CHECK(!B.MoveUnit(UnitId{1}, {4, 0}).has_value());   // out of range
	CHECK(!B.MoveUnit(UnitId{1}, {1, 0}).has_value());   // occupied by ally
	CHECK(B.FindUnit(UnitId{1})->Position == (GridCoord{0, 0}));  // unchanged on failure
}

int main()
{
	RangeOnOpenPlainUsesManhattanDistance();
	ForestCostsMore();
	RangeIsLimitedByMovement();
	WallsAreRoutedAround();
	EnemyBlocksButAllyIsPassedThrough();
	PathToListsCellsFromStartToDestination();
	BattleAddUnitValidates();
	BattleMoveUnitReturnsEventAndUpdatesPosition();
	BattleMoveUnitRejectsInvalidMoves();
	return TEST_MAIN_RESULT();
}
