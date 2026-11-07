// Checks that MapLoader compiles and works under UE's toolchain settings. Rule tests live in Tests/Core.

#include "Misc/AutomationTest.h"

#include "Map/MapLoader.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCommanderMapLoaderWiringTest,
	"Commander3D.Wiring.MapLoaderIsUsable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommanderMapLoaderWiringTest::RunTest(const FString& Parameters)
{
	using commander::application::LoadMap;
	using commander::application::MapData;
	using commander::application::MapLoadError;

	MapData Data;
	Data.Width = 2;
	Data.Height = 1;
	Data.Terrains = {{1, true}, {0, false}};
	Data.Cells = {0, 1};

	const auto Loaded = LoadMap(Data);
	TestTrue(TEXT("Valid map loads"), Loaded.IsOk());

	Data.Cells = {0, 2};
	const auto Rejected = LoadMap(Data);
	TestFalse(TEXT("Unknown terrain id is rejected"), Rejected.IsOk());
	if (!Rejected.IsOk())
	{
		TestTrue(TEXT("Error is UnknownTerrainId"), Rejected.Error() == MapLoadError::UnknownTerrainId);
	}
	return true;
}
