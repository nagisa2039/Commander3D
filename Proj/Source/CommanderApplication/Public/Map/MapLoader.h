#pragma once

#include <cstdint>
#include <utility>
#include <vector>

#include "Battle/BattleMap.h"
#include "Common/Result.h"

namespace commander::application
{

// Map description after file parsing. Format-agnostic: the Infrastructure layer fills it from JSON.
struct MapData
{
	std::int32_t Width = 0;
	std::int32_t Height = 0;
	std::vector<domain::TerrainSpec> Terrains;
	std::vector<domain::TerrainId> Cells; // row-major, one TerrainId per cell
};

enum class MapLoadError
{
	InvalidSize,
	EmptyTerrainTable,
	CellCountMismatch,
	UnknownTerrainId,
};

// Validates MapData and builds a BattleMap. Checks run in the order of the MapLoadError values.
inline Result<domain::BattleMap, MapLoadError> LoadMap(const MapData& Data)
{
	using Outcome = Result<domain::BattleMap, MapLoadError>;

	const auto Grid = domain::Grid::Create(Data.Width, Data.Height);
	if (!Grid.has_value())
	{
		return Outcome::Err(MapLoadError::InvalidSize);
	}
	if (Data.Terrains.empty())
	{
		return Outcome::Err(MapLoadError::EmptyTerrainTable);
	}
	if (Data.Cells.size() != Grid->CellCount())
	{
		return Outcome::Err(MapLoadError::CellCountMismatch);
	}
	for (const domain::TerrainId Id : Data.Cells)
	{
		if (Id >= Data.Terrains.size())
		{
			return Outcome::Err(MapLoadError::UnknownTerrainId);
		}
	}

	auto Map = domain::BattleMap::Create(*Grid, Data.Terrains, Data.Cells);
	if (!Map.has_value())
	{
		// Unreachable: every Create() precondition was checked above.
		return Outcome::Err(MapLoadError::CellCountMismatch);
	}
	return Outcome::Ok(std::move(*Map));
}

} // namespace commander::application
