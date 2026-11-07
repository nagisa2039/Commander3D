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

// A weapon's numbers. The unit can strike targets at Manhattan distance MinRange..MaxRange.
// MaxRange == 0 means "no weapon": the unit cannot attack or counter.
struct WeaponSpec
{
	std::int32_t Might = 0;
	std::int32_t Hit = 0; // Base hit rate in percent.
	std::int32_t MinRange = 0;
	std::int32_t MaxRange = 0;
};

// A character on the map.
struct BattleUnit
{
	UnitId Id;
	Faction Side = Faction::Player;
	GridCoord Position;
	std::int32_t Movement = 0; // Movement points spent by terrain MoveCost.
	bool HasMoved = false;     // Reset when the unit's faction phase begins.
	bool HasActed = false;     // True after Wait or Attack; the unit can do nothing more this phase.
	std::int32_t Hp = 1;       // Units are removed from the battle when Hp reaches 0.
	std::int32_t MaxHp = 1;
	std::int32_t Strength = 0;
	std::int32_t Skill = 0;
	std::int32_t Speed = 0;
	std::int32_t Defense = 0;
	WeaponSpec Weapon;
};

} // namespace commander::domain
