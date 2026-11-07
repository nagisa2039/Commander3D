#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include "Battle/Grid.h"

namespace commander::domain
{

// Index into a BattleMap's terrain table. Definitions come from master data.
using TerrainId = std::uint16_t;

struct TerrainSpec
{
	std::int32_t MoveCost = 1;
	bool Passable = true;
};

// A Grid plus the terrain of each cell. Terrain specs live in a table; cells refer to them by TerrainId.
class BattleMap
{
public:
	// Returns nullopt if the table is empty, Cells does not have one entry per grid cell (row-major),
	// or a cell refers to a TerrainId outside the table.
	static std::optional<BattleMap> Create(Grid InGrid, std::vector<TerrainSpec> Table, std::vector<TerrainId> Cells)
	{
		if (Table.empty() || Cells.size() != InGrid.CellCount())
		{
			return std::nullopt;
		}
		for (const TerrainId Id : Cells)
		{
			if (Id >= Table.size())
			{
				return std::nullopt;
			}
		}
		return BattleMap(InGrid, std::move(Table), std::move(Cells));
	}

	const Grid& GetGrid() const { return GridValue; }

	// Null when Coord is outside the grid.
	const TerrainSpec* TerrainAt(const GridCoord& Coord) const
	{
		if (!GridValue.Contains(Coord))
		{
			return nullptr;
		}
		return &TerrainTable[CellTerrain[GridValue.IndexOf(Coord)]];
	}

private:
	BattleMap(Grid InGrid, std::vector<TerrainSpec> Table, std::vector<TerrainId> Cells)
		: GridValue(InGrid), TerrainTable(std::move(Table)), CellTerrain(std::move(Cells))
	{
	}

	Grid GridValue;
	std::vector<TerrainSpec> TerrainTable;
	std::vector<TerrainId> CellTerrain;
};

} // namespace commander::domain
