#pragma once

#include <cstdint>

namespace commander::domain
{

// Position of a cell on the battle grid. X grows east, Y grows south.
struct GridCoord
{
	std::int32_t X = 0;
	std::int32_t Y = 0;

	friend constexpr bool operator==(const GridCoord&, const GridCoord&) = default;
};

// Number of orthogonal steps between two cells.
constexpr std::int32_t ManhattanDistance(const GridCoord& A, const GridCoord& B)
{
	const std::int32_t Dx = A.X - B.X;
	const std::int32_t Dy = A.Y - B.Y;
	return (Dx < 0 ? -Dx : Dx) + (Dy < 0 ? -Dy : Dy);
}

} // namespace commander::domain
