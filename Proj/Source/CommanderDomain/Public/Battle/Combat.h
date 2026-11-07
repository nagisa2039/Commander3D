#pragma once

#include <algorithm>
#include <cstdint>

#include "Battle/BattleUnit.h"

namespace commander::domain
{

// Damage of one hit from Attacker's weapon: Strength + Might - Defense, never below 0.
inline std::int32_t CalculateDamage(const BattleUnit& Attacker, const BattleUnit& Defender)
{
	return std::max(0, Attacker.Strength + Attacker.Weapon.Might - Defender.Defense);
}

// Hit chance in percent: weapon Hit + 2 * Skill - 2 * target Speed, clamped to 0..100.
inline std::int32_t CalculateHitRate(const BattleUnit& Attacker, const BattleUnit& Defender)
{
	return std::clamp(Attacker.Weapon.Hit + 2 * Attacker.Skill - 2 * Defender.Speed, 0, 100);
}

// Whether Unit's weapon reaches a target at the given Manhattan distance.
inline bool WeaponReaches(const BattleUnit& Unit, std::int32_t Distance)
{
	return Unit.Weapon.MaxRange > 0 && Distance >= Unit.Weapon.MinRange && Distance <= Unit.Weapon.MaxRange;
}

} // namespace commander::domain
