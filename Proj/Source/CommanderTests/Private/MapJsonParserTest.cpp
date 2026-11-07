// Infrastructure test: JSON text -> MapData -> BattleMap. Rule tests live in Tests/Core.

#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include "Infrastructure/MapJsonParser.h"
#include "Map/MapLoader.h"

namespace
{
using commander::application::MapData;
using commander::application::Result;

const TCHAR* ValidJson = TEXT(R"({
	"width": 3,
	"height": 2,
	"terrains": [
		{"moveCost": 1, "passable": true},
		{"moveCost": 2, "passable": true},
		{"moveCost": 0, "passable": false}
	],
	"cells": [
		[0, 1, 2],
		[1, 0, 0]
	]
})");

// Returns the parse error, or EMapJsonError::None when parsing succeeds.
EMapJsonError ErrorOf(const TCHAR* Json)
{
	const auto Parsed = FMapJsonParser::Parse(FString(Json));
	return Parsed.IsOk() ? EMapJsonError::None : Parsed.Error();
}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCommanderMapJsonParseValidTest,
	"Commander3D.Json.ParsesValidMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommanderMapJsonParseValidTest::RunTest(const FString& Parameters)
{
	const auto Parsed = FMapJsonParser::Parse(FString(ValidJson));
	if (!TestTrue(TEXT("Valid JSON parses"), Parsed.IsOk()))
	{
		return false;
	}
	const MapData& Data = Parsed.Value();
	TestEqual(TEXT("Width"), Data.Width, 3);
	TestEqual(TEXT("Height"), Data.Height, 2);
	TestEqual(TEXT("Terrain count"), static_cast<int32>(Data.Terrains.size()), 3);
	TestEqual(TEXT("Terrain 1 cost"), Data.Terrains[1].MoveCost, 2);
	TestFalse(TEXT("Terrain 2 impassable"), Data.Terrains[2].Passable);
	TestEqual(TEXT("Cell count"), static_cast<int32>(Data.Cells.size()), 6);
	TestEqual(TEXT("Cells are row-major: (1,1)"), static_cast<int32>(Data.Cells[4]), 0);
	TestEqual(TEXT("Cells are row-major: (0,1)"), static_cast<int32>(Data.Cells[3]), 1);

	const auto Loaded = commander::application::LoadMap(Data);
	TestTrue(TEXT("Parsed data loads into a BattleMap"), Loaded.IsOk());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCommanderMapJsonParseErrorsTest,
	"Commander3D.Json.RejectsInvalidInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommanderMapJsonParseErrorsTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Broken JSON"), ErrorOf(TEXT("{ not json")) == EMapJsonError::InvalidJson);
	TestTrue(TEXT("Empty string"), ErrorOf(TEXT("")) == EMapJsonError::InvalidJson);
	TestTrue(TEXT("Missing width"),
		ErrorOf(TEXT(R"({"height":1,"terrains":[{"moveCost":1,"passable":true}],"cells":[[0]]})")) == EMapJsonError::InvalidField);
	TestTrue(TEXT("Missing terrains"),
		ErrorOf(TEXT(R"({"width":1,"height":1,"cells":[[0]]})")) == EMapJsonError::InvalidField);
	TestTrue(TEXT("Terrain without passable"),
		ErrorOf(TEXT(R"({"width":1,"height":1,"terrains":[{"moveCost":1}],"cells":[[0]]})")) == EMapJsonError::InvalidField);
	TestTrue(TEXT("Cells is not an array of arrays"),
		ErrorOf(TEXT(R"({"width":1,"height":1,"terrains":[{"moveCost":1,"passable":true}],"cells":[0]})")) == EMapJsonError::InvalidField);
	TestTrue(TEXT("Negative terrain id"),
		ErrorOf(TEXT(R"({"width":1,"height":1,"terrains":[{"moveCost":1,"passable":true}],"cells":[[-1]]})")) == EMapJsonError::InvalidField);
	TestTrue(TEXT("Terrain id above uint16"),
		ErrorOf(TEXT(R"({"width":1,"height":1,"terrains":[{"moveCost":1,"passable":true}],"cells":[[65536]]})")) == EMapJsonError::InvalidField);
	TestTrue(TEXT("Row shorter than width"),
		ErrorOf(TEXT(R"({"width":2,"height":1,"terrains":[{"moveCost":1,"passable":true}],"cells":[[0]]})")) == EMapJsonError::RowShapeMismatch);
	TestTrue(TEXT("Fewer rows than height"),
		ErrorOf(TEXT(R"({"width":1,"height":2,"terrains":[{"moveCost":1,"passable":true}],"cells":[[0]]})")) == EMapJsonError::RowShapeMismatch);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCommanderMapJsonSampleFileTest,
	"Commander3D.Json.LoadsSampleMapFile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommanderMapJsonSampleFileTest::RunTest(const FString& Parameters)
{
	const FString Path = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data/Maps/test_map.json"));
	FString Text;
	if (!TestTrue(TEXT("Sample map file is readable"), FFileHelper::LoadFileToString(Text, *Path)))
	{
		return false;
	}
	const auto Parsed = FMapJsonParser::Parse(Text);
	if (!TestTrue(TEXT("Sample map parses"), Parsed.IsOk()))
	{
		return false;
	}
	TestTrue(TEXT("Sample map passes MapLoader"), commander::application::LoadMap(Parsed.Value()).IsOk());
	return true;
}
