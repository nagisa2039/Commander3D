#pragma once

#include <algorithm>
#include <cstdint>

#include "Battle/BattleUnit.h"
#include "Battle/Progression.h"

namespace commander::domain
{

// Damage of one hit from Attacker's weapon: Strength + Might - Defense, never below 0.
inline std::int32_t CalculateDamage(const BattleUnit& Attacker, const BattleUnit& Defender)
{
	return std::max(0, Attacker.Strength + Attacker.Weapon.Might - Defender.Defense);
}

// Whether the equipped weapon exists, is wieldable and has not broken (defined below).
inline bool HasUsableWeapon(const BattleUnit& Unit);

// Hit chance in percent, clamped to 0..100:
//   weapon Hit + RankHit per attacker rank + 2 * Skill
//   - (2 * target Speed + the target's weapon Avoid + its RankAvoid per target rank).
// The target's weapon only counts while it is usable (a broken or missing weapon gives no avoid).
inline std::int32_t CalculateHitRate(const BattleUnit& Attacker, const BattleUnit& Defender)
{
	const std::int32_t Hit = Attacker.Weapon.Hit + Attacker.Weapon.RankHit * WieldRank(Attacker, Attacker.Weapon) + 2 * Attacker.Skill;
	std::int32_t Avoid = 2 * Defender.Speed;
	if (HasUsableWeapon(Defender))
	{
		Avoid += Defender.Weapon.Avoid + Defender.Weapon.RankAvoid * WieldRank(Defender, Defender.Weapon);
	}
	return std::clamp(Hit - Avoid, 0, 100);
}

// Whether a weapon's numbers make sense: ranges not negative and ordered, Hit within 0..100, Durability not
// negative and, for a durable weapon, Uses within 0..Durability; Type within 0..MaxWeaponTypes,
// RequiredRank within 0..MaxWeaponRank, Avoid within -100..100 and RankHit / RankAvoid within 0..50.
inline bool IsWeaponConsistent(const WeaponSpec& W, std::int32_t Uses)
{
	const bool bUsesOk = W.Durability == 0 || (Uses >= 0 && Uses <= W.Durability);
	const bool bTypeOk = W.Type >= 0 && W.Type <= MaxWeaponTypes && W.RequiredRank >= 0 && W.RequiredRank <= MaxWeaponRank
		&& W.Avoid >= -100 && W.Avoid <= 100 && W.RankHit >= 0 && W.RankHit <= 50 && W.RankAvoid >= 0 && W.RankAvoid <= 50;
	return W.MinRange >= 0 && W.MaxRange >= 0 && W.MinRange <= W.MaxRange && W.Hit >= 0 && W.Hit <= 100 && W.Durability >= 0
		&& bUsesOk && bTypeOk;
}

// Whether Unit could strike with Weapon (which has Uses left): it exists (MaxRange > 0), the unit has the rank
// for it, and it has not broken. Weapons with Durability 0 never break.
inline bool CanUseWeapon(const BattleUnit& Unit, const WeaponSpec& Weapon, std::int32_t Uses)
{
	return Weapon.MaxRange > 0 && CanWield(Unit, Weapon) && (Weapon.Durability == 0 || Uses > 0);
}

// The equipped weapon exists, is wieldable by the unit and has not broken.
inline bool HasUsableWeapon(const BattleUnit& Unit)
{
	return CanUseWeapon(Unit, Unit.Weapon, Unit.WeaponUses);
}

// Whether Unit's weapon can strike a target at the given Manhattan distance.
inline bool WeaponReaches(const BattleUnit& Unit, std::int32_t Distance)
{
	return HasUsableWeapon(Unit) && Distance >= Unit.Weapon.MinRange && Distance <= Unit.Weapon.MaxRange;
}

} // namespace commander::domain
