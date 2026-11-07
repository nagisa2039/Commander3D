#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

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

// Weapon types are small ids 1..MaxWeaponTypes (0 = untyped: no rank needed, no training). Ranks are 0..MaxWeaponRank.
inline constexpr std::int32_t MaxWeaponTypes = 8;
inline constexpr std::int32_t MaxWeaponRank = 3;

// A weapon's numbers. The unit can strike targets at Manhattan distance MinRange..MaxRange.
// MaxRange == 0 means "no weapon": the unit cannot attack or counter.
struct WeaponSpec
{
	std::int32_t Might = 0;
	std::int32_t Hit = 0; // Base hit rate in percent.
	std::int32_t MinRange = 0;
	std::int32_t MaxRange = 0;
	std::int32_t Durability = 0;   // Uses before it breaks. 0 = never breaks.
	std::int32_t Type = 0;         // Weapon type id (0 = untyped).
	std::int32_t RequiredRank = 0; // The wielder needs at least this rank in Type.
	std::int32_t Avoid = 0;        // Points taken off the hit chance of attacks against the wielder (-100..100; negative = easier to hit).
	std::int32_t RankHit = 0;      // Extra hit chance per wielder rank in Type (0..50).
	std::int32_t RankAvoid = 0;    // Extra avoid per wielder rank in Type (0..50).
};

// Chance (percent, 0..100) that each stat rises by 1 on a level-up.
struct GrowthRates
{
	std::int32_t Hp = 0;
	std::int32_t Strength = 0;
	std::int32_t Skill = 0;
	std::int32_t Speed = 0;
	std::int32_t Defense = 0;
};

// Highest value each stat can reach by level-ups. 0 = no cap. Hp is compared with MaxHp.
struct StatCaps
{
	std::int32_t Hp = 0;
	std::int32_t Strength = 0;
	std::int32_t Skill = 0;
	std::int32_t Speed = 0;
	std::int32_t Defense = 0;
};

// Flat stat gains (promotion).
struct StatBonus
{
	std::int32_t Hp = 0;
	std::int32_t Strength = 0;
	std::int32_t Skill = 0;
	std::int32_t Speed = 0;
	std::int32_t Defense = 0;
	std::int32_t Movement = 0;
};

// One class change a unit can make: available once it reaches MinLevel (or, with a promotion seal, the seal's
// level). Level and experience reset to 1 and 0; Growth and Caps replace the unit's. Name identifies the new
// class for display and for the Presentation.
struct PromotionSpec
{
	std::string Name;
	std::int32_t MinLevel = 1;
	StatBonus Bonus;
	GrowthRates Growth;
	StatCaps Caps;
};

enum class ItemKind : std::uint8_t
{
	Weapon,    // a spare weapon; Weapon / Uses hold its numbers and remaining uses
	Potion,    // heals the user by Amount Hp; Uses = doses left
	RepairKit, // restores the equipped weapon to full durability; Uses = charges left
	Staff,     // heals an ally within Range by Amount Hp; Uses = charges left
	PromotionSeal, // lets a unit of level >= Amount (1..MaxLevel) change class before its normal level; Uses = charges left
};

// One inventory slot (or shop stock entry). Only the fields of its Kind are meaningful.
struct ItemSlot
{
	ItemKind Kind = ItemKind::Weapon;
	WeaponSpec Weapon;       // Weapon
	std::int32_t Uses = 0;   // Weapon: uses left (ignored when Weapon.Durability == 0); others: charges left (> 0)
	std::int32_t Amount = 0; // Potion / Staff: Hp restored (> 0)
	std::int32_t Range = 0;  // Staff: farthest Manhattan distance it reaches (>= 1)
	std::int32_t Price = 0;  // Gold to buy it at a shop; it sells for half. 0 = not for sale
};

inline constexpr std::size_t MaxItemSlots = 5;
inline constexpr std::size_t MaxPromotionOptions = 4;

// A character on the map.
struct BattleUnit
{
	UnitId Id;
	Faction Side = Faction::Player;
	GridCoord Position;
	std::int32_t Movement = 0; // Movement points spent by terrain MoveCost.
	bool HasMoved = false;     // Reset when the unit's faction phase begins.
	bool HasActed = false;     // True after Wait, Attack, UseItem, ...; the unit can do nothing more this phase.
	std::int32_t Hp = 1;       // Units are removed from the battle when Hp reaches 0.
	std::int32_t MaxHp = 1;
	std::int32_t Strength = 0;
	std::int32_t Skill = 0;
	std::int32_t Speed = 0;
	std::int32_t Defense = 0;
	WeaponSpec Weapon;         // The equipped weapon.
	std::int32_t WeaponUses = 0; // Uses left; only meaningful when Weapon.Durability > 0. 0 there means broken.
	bool IsBoss = false;       // Used by the "defeat the boss" victory condition.
	std::int32_t Level = 1;
	std::int32_t Experience = 0; // 0..99 toward the next level. Only player units gain experience.
	GrowthRates Growth;
	std::vector<ItemSlot> Items; // At most MaxItemSlots.
	StatCaps Caps;
	std::vector<PromotionSpec> Promotions; // The class changes this unit can still choose from (empty = none; at most MaxPromotionOptions).
	std::array<std::int32_t, MaxWeaponTypes + 1> WeaponPoints{}; // Training per weapon type (index = type id; 0 unused).
};

} // namespace commander::domain
