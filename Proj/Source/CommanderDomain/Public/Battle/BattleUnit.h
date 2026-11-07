#pragma once

#include <cstdint>

#include "Battle/GridCoord.h"

namespace commander::domain
{

struct UnitId
{
	std::uint32_t Value = 0;

	friend constexpr bool operator==(const UnitId&, const UnitId&) = default;
};

enum class Faction : std::uint8_t
{
	Player,
	Enemy,
};

// A character on the map.
struct BattleUnit
{
	UnitId Id;
	Faction Side = Faction::Player;
	GridCoord Position;
	std::int32_t Movement = 0; // Movement points spent by terrain MoveCost.
};

} // namespace commander::domain
