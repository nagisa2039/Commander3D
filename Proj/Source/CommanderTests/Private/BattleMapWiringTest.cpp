// Checks that Grid / BattleMap compile and work under UE's toolchain settings. Rule tests live in Tests/Core.

#include "Misc/AutomationTest.h"

#include <vector>

#include "Battle/BattleMap.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCommanderBattleMapWiringTest,
	"Commander3D.Wiring.BattleMapIsUsable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommanderBattleMapWiringTest::RunTest(const FString& Parameters)
{
	using commander::domain::BattleMap;
	using commander::domain::Grid;
	using commander::domain::GridCoord;
	using commander::domain::TerrainId;
	using commander::domain::TerrainSpec;

	const auto G = Grid::Create(2, 2);
	TestTrue(TEXT("Grid created"), G.has_value());
	TestFalse(TEXT("Zero-size grid rejected"), Grid::Create(0, 2).has_value());

	const std::vector<TerrainSpec> Table = {{1, true}, {0, false}};
	const std::vector<TerrainId> Cells = {0, 1, 0, 0};
	const auto Map = BattleMap::Create(*G, Table, Cells);
	TestTrue(TEXT("BattleMap created"), Map.has_value());
	if (!Map.has_value())
	{
		return false;
	}

	TestFalse(TEXT("Wall is impassable"), Map->TerrainAt(GridCoord{1, 0})->Passable);
	TestTrue(TEXT("Outside grid is null"), Map->TerrainAt(GridCoord{2, 0}) == nullptr);
	return true;
}
