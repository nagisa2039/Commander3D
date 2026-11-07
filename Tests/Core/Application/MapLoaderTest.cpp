#include "Map/MapLoader.h"
#include "TestSupport.h"

using commander::application::LoadMap;
using commander::application::MapData;
using commander::application::MapLoadError;
using commander::domain::GridCoord;

static MapData ValidData()
{
	// 3x2: plain forest wall / forest plain plain
	MapData Data;
	Data.Width = 3;
	Data.Height = 2;
	Data.Terrains = {{1, true}, {2, true}, {0, false}};
	Data.Cells = {0, 1, 2, 1, 0, 0};
	return Data;
}

static void LoadsValidData()
{
	const auto Result = LoadMap(ValidData());
	CHECK(Result.IsOk());
	if (!Result.IsOk())
	{
		return;
	}
	const auto& Map = Result.Value();
	CHECK(Map.GetGrid().Width() == 3);
	CHECK(Map.GetGrid().Height() == 2);
	CHECK(!Map.TerrainAt(GridCoord{2, 0})->Passable);
	CHECK(Map.TerrainAt(GridCoord{1, 0})->MoveCost == 2);
}

static void RejectsInvalidSize()
{
	MapData Data = ValidData();
	Data.Width = 0;
	const auto Result = LoadMap(Data);
	CHECK(!Result.IsOk());
	CHECK(!Result.IsOk() && Result.Error() == MapLoadError::InvalidSize);

	Data = ValidData();
	Data.Height = -1;
	CHECK(!LoadMap(Data).IsOk() && LoadMap(Data).Error() == MapLoadError::InvalidSize);
}

static void RejectsEmptyTerrainTable()
{
	MapData Data = ValidData();
	Data.Terrains.clear();
	const auto Result = LoadMap(Data);
	CHECK(!Result.IsOk() && Result.Error() == MapLoadError::EmptyTerrainTable);
}

static void RejectsCellCountMismatch()
{
	MapData Data = ValidData();
	Data.Cells.pop_back();
	const auto Result = LoadMap(Data);
	CHECK(!Result.IsOk() && Result.Error() == MapLoadError::CellCountMismatch);
}

static void RejectsUnknownTerrainId()
{
	MapData Data = ValidData();
	Data.Cells[5] = 3;
	const auto Result = LoadMap(Data);
	CHECK(!Result.IsOk() && Result.Error() == MapLoadError::UnknownTerrainId);
}

static void InvalidSizeIsReportedBeforeCellCount()
{
	MapData Data = ValidData();
	Data.Width = 0;
	Data.Cells.clear();
	const auto Result = LoadMap(Data);
	CHECK(!Result.IsOk() && Result.Error() == MapLoadError::InvalidSize);
}

int main()
{
	LoadsValidData();
	RejectsInvalidSize();
	RejectsEmptyTerrainTable();
	RejectsCellCountMismatch();
	RejectsUnknownTerrainId();
	InvalidSizeIsReportedBeforeCellCount();
	return TEST_MAIN_RESULT();
}
