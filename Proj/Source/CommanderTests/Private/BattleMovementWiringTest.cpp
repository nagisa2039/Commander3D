// Checks that Battle / MovementRange compile and work under UE's toolchain settings. Rule tests live in Tests/Core.

#include "Misc/AutomationTest.h"

#include <vector>

#include "Battle/Battle.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCommanderBattleMovementWiringTest,
	"Commander3D.Wiring.BattleMovementIsUsable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommanderBattleMovementWiringTest::RunTest(const FString& Parameters)
{
	using namespace commander::domain;

	const std::vector<TerrainSpec> Table = {{1, true}, {0, false}};
	const std::vector<TerrainId> Cells = {0, 0, 0, 0, 1, 0};
	const auto Map = BattleMap::Create(*Grid::Create(3, 2), Table, Cells);
	TestTrue(TEXT("BattleMap created"), Map.has_value());
	if (!Map.has_value())
	{
		return false;
	}

	Battle B(*Map);
	TestTrue(TEXT("Unit added"), B.AddUnit(BattleUnit{UnitId{1}, Faction::Player, GridCoord{0, 0}, 3}));
	const auto Event = B.MoveUnit(UnitId{1}, GridCoord{2, 0});
	TestTrue(TEXT("Move succeeds"), Event.has_value());
	TestFalse(TEXT("Move onto wall rejected"), B.MoveUnit(UnitId{1}, GridCoord{1, 1}).has_value());
	return true;
}
