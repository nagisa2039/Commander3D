#pragma once

#include <algorithm>
#include <cstdint>

#include "Battle/BattleUnit.h"
#include "Battle/RandomSource.h"

namespace commander::domain
{

inline constexpr std::int32_t MaxLevel = 20;
inline constexpr std::int32_t ExperiencePerLevel = 100;

// Training points (one per strike) needed for weapon ranks 0..MaxWeaponRank.
inline constexpr std::int32_t RankThreshold[MaxWeaponRank + 1] = {0, 20, 50, 90};
inline constexpr std::int32_t MaxWeaponPoints = 999;

inline std::int32_t WeaponRankOf(std::int32_t Points)
{
	std::int32_t Rank = 0;
	while (Rank < MaxWeaponRank && Points >= RankThreshold[Rank + 1])
	{
		++Rank;
	}
	return Rank;
}

// Unit's rank in Weapon's type (0 for untyped weapons or an invalid type).
inline std::int32_t WieldRank(const BattleUnit& Unit, const WeaponSpec& Weapon)
{
	if (Weapon.Type < 1 || Weapon.Type > MaxWeaponTypes)
	{
		return 0;
	}
	return WeaponRankOf(Unit.WeaponPoints[static_cast<std::size_t>(Weapon.Type)]);
}

// Experience a player healer earns for a staff use: 8, plus half the Hp actually restored.
inline std::int32_t CalculateStaffExperience(std::int32_t HpHealed)
{
	return 8 + HpHealed / 2;
}

// Whether Unit has the rank to wield Weapon. Untyped weapons need none.
inline bool CanWield(const BattleUnit& Unit, const WeaponSpec& Weapon)
{
	if (Weapon.Type == 0)
	{
		return true;
	}
	if (Weapon.Type < 1 || Weapon.Type > MaxWeaponTypes)
	{
		return false;
	}
	return WeaponRankOf(Unit.WeaponPoints[static_cast<std::size_t>(Weapon.Type)]) >= Weapon.RequiredRank;
}

// Event: a unit gained experience from a fight.
struct ExperienceGained
{
	UnitId Unit;
	std::int32_t Amount = 0;
};

// Event: a unit reached NewLevel; the *Gain fields are 0 or 1 each.
struct LevelUp
{
	UnitId Unit;
	std::int32_t NewLevel = 1;
	std::int32_t HpGain = 0;
	std::int32_t StrengthGain = 0;
	std::int32_t SkillGain = 0;
	std::int32_t SpeedGain = 0;
	std::int32_t DefenseGain = 0;
};

// Experience Self earns from a fight with Opponent: 10, +3 per level the opponent is above Self, +20 for a kill.
inline std::int32_t CalculateExperience(const BattleUnit& Self, const BattleUnit& Opponent, bool Killed)
{
	return 10 + 3 * std::max(0, Opponent.Level - Self.Level) + (Killed ? 20 : 0);
}

// Adds experience to Unit (no effect at MaxLevel). Returns true if it reached a new level, filling Result;
// each stat rises by 1 with its growth rate as the percent chance (one roll per stat: Hp, Strength, Skill,
// Speed, Defense, in that order). A gain is below ExperiencePerLevel, so at most one level is gained.
inline bool AddExperience(BattleUnit& Unit, std::int32_t Amount, IRandomSource& Rng, LevelUp& Result)
{
	if (Unit.Level >= MaxLevel)
	{
		return false;
	}
	Unit.Experience += Amount;
	if (Unit.Experience < ExperiencePerLevel)
	{
		return false;
	}
	Unit.Experience -= ExperiencePerLevel;
	++Unit.Level;
	if (Unit.Level >= MaxLevel)
	{
		Unit.Experience = 0;
	}

	// A roll is always made for every stat (so the random sequence does not depend on the caps); a success
	// only counts while the stat is below its cap (0 = no cap).
	const auto Roll = [&Rng](std::int32_t Rate, std::int32_t Current, std::int32_t Cap) {
		const bool bSucceeded = Rng.NextPercent() < Rate;
		return (bSucceeded && (Cap == 0 || Current < Cap)) ? 1 : 0;
	};
	Result = LevelUp{};
	Result.Unit = Unit.Id;
	Result.NewLevel = Unit.Level;
	Result.HpGain = Roll(Unit.Growth.Hp, Unit.MaxHp, Unit.Caps.Hp);
	Result.StrengthGain = Roll(Unit.Growth.Strength, Unit.Strength, Unit.Caps.Strength);
	Result.SkillGain = Roll(Unit.Growth.Skill, Unit.Skill, Unit.Caps.Skill);
	Result.SpeedGain = Roll(Unit.Growth.Speed, Unit.Speed, Unit.Caps.Speed);
	Result.DefenseGain = Roll(Unit.Growth.Defense, Unit.Defense, Unit.Caps.Defense);
	Unit.MaxHp += Result.HpGain;
	Unit.Hp += Result.HpGain;
	Unit.Strength += Result.StrengthGain;
	Unit.Skill += Result.SkillGain;
	Unit.Speed += Result.SpeedGain;
	Unit.Defense += Result.DefenseGain;
	return true;
}

} // namespace commander::domain
