#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <queue>
#include <utility>
#include <vector>

#include "Battle/BattleMap.h"
#include "Battle/BattleUnit.h"

namespace commander::domain
{

// Cells a unit can reach this move, with the cheapest cost and route to each.
class MovementRange
{
public:
	MovementRange(Grid InGrid, GridCoord InStart, std::vector<std::int32_t> InCost, std::vector<std::size_t> InPrev,
		std::vector<bool> InCanStop)
		: GridValue(InGrid), Start(InStart), Cost(std::move(InCost)), Prev(std::move(InPrev)), Stop(std::move(InCanStop))
	{
	}

	// Cheapest cost to enter Coord (0 at the start), or -1 if unreachable or outside the grid.
	// Cells occupied by allies are reachable (passed through) but CanStopAt is false for them.
	std::int32_t CostTo(const GridCoord& Coord) const
	{
		return GridValue.Contains(Coord) ? Cost[GridValue.IndexOf(Coord)] : -1;
	}

	bool CanStopAt(const GridCoord& Coord) const
	{
		return GridValue.Contains(Coord) && Stop[GridValue.IndexOf(Coord)];
	}

	// Route from the start to Coord inclusive of both ends. Empty if Coord is unreachable.
	std::vector<GridCoord> PathTo(const GridCoord& Coord) const
	{
		std::vector<GridCoord> Path;
		if (CostTo(Coord) < 0)
		{
			return Path;
		}
		std::size_t Index = GridValue.IndexOf(Coord);
		const std::size_t StartIndex = GridValue.IndexOf(Start);
		while (true)
		{
			Path.push_back(GridCoord{static_cast<std::int32_t>(Index % static_cast<std::size_t>(GridValue.Width())),
				static_cast<std::int32_t>(Index / static_cast<std::size_t>(GridValue.Width()))});
			if (Index == StartIndex)
			{
				break;
			}
			Index = Prev[Index];
		}
		return {Path.rbegin(), Path.rend()};
	}

	// All cells the unit may end its move on (includes the start).
	std::vector<GridCoord> StoppableCells() const
	{
		std::vector<GridCoord> Result;
		for (std::int32_t Y = 0; Y < GridValue.Height(); ++Y)
		{
			for (std::int32_t X = 0; X < GridValue.Width(); ++X)
			{
				if (Stop[GridValue.IndexOf({X, Y})])
				{
					Result.push_back({X, Y});
				}
			}
		}
		return Result;
	}

private:
	Grid GridValue;
	GridCoord Start;
	std::vector<std::int32_t> Cost;
	std::vector<std::size_t> Prev;
	std::vector<bool> Stop;
};

// Dijkstra over 4-adjacent cells. Enemy-faction units block; same-faction units can be passed but not stopped on.
// Mover's own cell is the start. Units is every unit on the map (including Mover).
inline MovementRange CalculateMovementRange(const BattleMap& Map, const std::vector<BattleUnit>& Units, const BattleUnit& Mover)
{
	const Grid& G = Map.GetGrid();
	std::vector<std::int32_t> Cost(G.CellCount(), -1);
	std::vector<std::size_t> Prev(G.CellCount(), 0);
	std::vector<bool> Stop(G.CellCount(), false);

	std::vector<const BattleUnit*> Occupant(G.CellCount(), nullptr);
	for (const BattleUnit& U : Units)
	{
		if (G.Contains(U.Position) && U.Id != Mover.Id)
		{
			Occupant[G.IndexOf(U.Position)] = &U;
		}
	}

	if (!G.Contains(Mover.Position))
	{
		return MovementRange(G, Mover.Position, std::move(Cost), std::move(Prev), std::move(Stop));
	}

	using Entry = std::pair<std::int32_t, std::size_t>; // (cost, cell index)
	std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> Open;
	const std::size_t StartIndex = G.IndexOf(Mover.Position);
	Cost[StartIndex] = 0;
	Open.push({0, StartIndex});

	static constexpr GridCoord Offsets[4] = {{0, -1}, {1, 0}, {0, 1}, {-1, 0}};
	while (!Open.empty())
	{
		const auto [CurrentCost, CurrentIndex] = Open.top();
		Open.pop();
		if (CurrentCost != Cost[CurrentIndex])
		{
			continue; // stale entry
		}
		const GridCoord Current{static_cast<std::int32_t>(CurrentIndex % static_cast<std::size_t>(G.Width())),
			static_cast<std::int32_t>(CurrentIndex / static_cast<std::size_t>(G.Width()))};
		for (const GridCoord& Offset : Offsets)
		{
			const GridCoord Next{Current.X + Offset.X, Current.Y + Offset.Y};
			const TerrainSpec* Terrain = Map.TerrainAt(Next);
			if (Terrain == nullptr || !Terrain->Passable)
			{
				continue;
			}
			const std::size_t NextIndex = G.IndexOf(Next);
			const BattleUnit* Other = Occupant[NextIndex];
			if (Other != nullptr && Other->Side != Mover.Side)
			{
				continue;
			}
			const std::int32_t NextCost = CurrentCost + Terrain->MoveCost;
			if (NextCost > Mover.Movement)
			{
				continue;
			}
			if (Cost[NextIndex] < 0 || NextCost < Cost[NextIndex])
			{
				Cost[NextIndex] = NextCost;
				Prev[NextIndex] = CurrentIndex;
				Open.push({NextCost, NextIndex});
			}
		}
	}

	for (std::size_t I = 0; I < Stop.size(); ++I)
	{
		Stop[I] = Cost[I] >= 0 && Occupant[I] == nullptr;
	}
	return MovementRange(G, Mover.Position, std::move(Cost), std::move(Prev), std::move(Stop));
}

} // namespace commander::domain
