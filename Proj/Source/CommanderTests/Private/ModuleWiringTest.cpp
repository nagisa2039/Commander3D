// Checks that the UE-free modules are usable from UE code. Rule tests live in Tests/Core.

#include "Misc/AutomationTest.h"

#include "Battle/GridCoord.h"
#include "Common/Result.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCommanderModuleWiringTest,
	"Commander3D.Wiring.DomainAndApplicationAreUsable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommanderModuleWiringTest::RunTest(const FString& Parameters)
{
	using commander::application::Result;
	using commander::domain::GridCoord;
	using commander::domain::ManhattanDistance;

	TestEqual(TEXT("Manhattan distance"), ManhattanDistance(GridCoord{0, 0}, GridCoord{3, 4}), 7);

	const auto Ok = Result<int, int>::Ok(1);
	const auto Err = Result<int, int>::Err(1);
	TestTrue(TEXT("Ok is ok"), Ok.IsOk());
	TestFalse(TEXT("Err is not ok"), Err.IsOk());
	return true;
}
