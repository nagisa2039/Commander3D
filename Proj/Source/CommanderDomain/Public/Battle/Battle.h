#pragma once

#include <optional>
#include <utility>
#include <vector>

#include "Battle/BattleMap.h"
#include "Battle/BattleUnit.h"
#include "Battle/MovementRange.h"

namespace commander::domain
{

// Event: a unit moved along Path (start and destination inclusive).
struct UnitMoved
{
	UnitId Unit;
	std::vector<GridCoord> Path;
};

// One map's state: terrain plus the units on it. Aggregate root.
class Battle
{
public:
	explicit Battle(BattleMap InMap) : Map(std::move(InMap)) {}

	const BattleMap& GetMap() const { return Map; }
	const std::vector<BattleUnit>& Units() const { return UnitList; }

	// Fails (false) on duplicate id, negative movement, a cell outside the grid, impassable or occupied.
	bool AddUnit(const BattleUnit& Unit)
	{
		const TerrainSpec* Terrain = Map.TerrainAt(Unit.Position);
		if (Terrain == nullptr || !Terrain->Passable || Unit.Movement < 0 || FindUnit(Unit.Id) != nullptr
			|| UnitAt(Unit.Position) != nullptr)
		{
			return false;
		}
		UnitList.push_back(Unit);
		return true;
	}

	const BattleUnit* FindUnit(UnitId Id) const
	{
		for (const BattleUnit& U : UnitList)
		{
			if (U.Id == Id)
			{
				return &U;
			}
		}
		return nullptr;
	}

	const BattleUnit* UnitAt(const GridCoord& Coord) const
	{
		for (const BattleUnit& U : UnitList)
		{
			if (U.Position == Coord)
			{
				return &U;
			}
		}
		return nullptr;
	}

	// Returns nullopt (state unchanged) if the unit is unknown or Destination is not a cell it can stop on.
	// A move to the unit's own cell is valid (path of one cell).
	std::optional<UnitMoved> MoveUnit(UnitId Id, const GridCoord& Destination)
	{
		BattleUnit* Unit = FindMutable(Id);
		if (Unit == nullptr)
		{
			return std::nullopt;
		}
		const MovementRange Range = CalculateMovementRange(Map, UnitList, *Unit);
		if (!Range.CanStopAt(Destination))
		{
			return std::nullopt;
		}
		UnitMoved Event{Id, Range.PathTo(Destination)};
		Unit->Position = Destination;
		return Event;
	}

private:
	BattleUnit* FindMutable(UnitId Id)
	{
		for (BattleUnit& U : UnitList)
		{
			if (U.Id == Id)
			{
				return &U;
			}
		}
		return nullptr;
	}

	BattleMap Map;
	std::vector<BattleUnit> UnitList;
};

} // namespace commander::domain
