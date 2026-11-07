// Checks that BattleSetup / BattleSession compile and work under UE's toolchain settings. Rule tests live in Tests/Core.

#include "Misc/AutomationTest.h"

#include "Battle/BattleSession.h"
#include "Battle/BattleSetup.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCommanderBattleSessionWiringTest,
	"Commander3D.Wiring.BattleSessionIsUsable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommanderBattleSessionWiringTest::RunTest(const FString& Parameters)
{
	using namespace commander::application;
	using namespace commander::domain;

	struct FZeroRandom : IRandomSource
	{
		int32 NextPercent() override { return 0; }
	};

	BattleSetup Setup;
	Setup.Map.Width = 3;
	Setup.Map.Height = 1;
	Setup.Map.Terrains = {{1, true}};
	Setup.Map.Cells = {0, 0, 0};
	Setup.Units = {BattleUnit{UnitId{1}, Faction::Player, GridCoord{0, 0}, 2}, BattleUnit{UnitId{2}, Faction::Enemy, GridCoord{2, 0}, 2}};

	auto Started = StartBattle(Setup);
	TestTrue(TEXT("Battle started"), Started.IsOk());
	if (!Started.IsOk())
	{
		return false;
	}

	FZeroRandom Rng;
	BattleSession Session(Started.Value(), Rng);
	const CommandResult Moved = Session.Execute(MoveUnitCommand{UnitId{1}, GridCoord{1, 0}});
	TestTrue(TEXT("Move command ok"), Moved.IsOk() && std::holds_alternative<UnitMoved>(Moved.Value()));
	const CommandResult Rejected = Session.Execute(MoveUnitCommand{UnitId{1}, GridCoord{0, 0}});
	TestTrue(TEXT("Second move rejected"), !Rejected.IsOk() && Rejected.Error() == CommandError::Rejected);

	Setup.Units.clear();
	TestTrue(TEXT("No units rejected"), StartBattle(Setup).Error() == BattleSetupError::NoPlayerUnit);
	return true;
}
