#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

#include "Battle/GridCoord.h"

namespace commander::domain
{

// Rectangular battle grid. Cells are addressed by GridCoord, (0,0) is the north-west corner.
class Grid
{
public:
	// Returns nullopt unless both dimensions are positive.
	static std::optional<Grid> Create(std::int32_t Width, std::int32_t Height)
	{
		if (Width <= 0 || Height <= 0)
		{
			return std::nullopt;
		}
		return Grid(Width, Height);
	}

	std::int32_t Width() const { return WidthValue; }
	std::int32_t Height() const { return HeightValue; }
	std::size_t CellCount() const { return static_cast<std::size_t>(WidthValue) * static_cast<std::size_t>(HeightValue); }

	bool Contains(const GridCoord& Coord) const
	{
		return Coord.X >= 0 && Coord.X < WidthValue && Coord.Y >= 0 && Coord.Y < HeightValue;
	}

	// Row-major index. Requires Contains(Coord).
	std::size_t IndexOf(const GridCoord& Coord) const
	{
		return static_cast<std::size_t>(Coord.Y) * static_cast<std::size_t>(WidthValue) + static_cast<std::size_t>(Coord.X);
	}

private:
	Grid(std::int32_t InWidth, std::int32_t InHeight) : WidthValue(InWidth), HeightValue(InHeight) {}

	std::int32_t WidthValue;
	std::int32_t HeightValue;
};

} // namespace commander::domain
