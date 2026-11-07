#pragma once

#include "Battle/BattleUnit.h"
#include "Battle/Combat.h"
#include "Battle/Progression.h"

namespace commander::domain
{

inline bool IsItemSlotValid(const ItemSlot& Slot)
{
	if (Slot.Price < 0)
	{
		return false;
	}
	switch (Slot.Kind)
	{
	case ItemKind::Weapon:
		return Slot.Weapon.MaxRange > 0 && IsWeaponConsistent(Slot.Weapon, Slot.Uses);
	case ItemKind::Potion:
		return Slot.Uses > 0 && Slot.Amount > 0;
	case ItemKind::RepairKit:
		return Slot.Uses > 0;
	case ItemKind::Staff:
		return Slot.Uses > 0 && Slot.Amount > 0 && Slot.Range >= 1;
	case ItemKind::PromotionSeal:
		return Slot.Uses > 0 && Slot.Amount >= 1 && Slot.Amount <= MaxLevel;
	}
	return false;
}

inline bool AreRatesValid(const GrowthRates& G)
{
	const auto RateOk = [](std::int32_t Rate) { return Rate >= 0 && Rate <= 100; };
	return RateOk(G.Hp) && RateOk(G.Strength) && RateOk(G.Skill) && RateOk(G.Speed) && RateOk(G.Defense);
}

inline bool AreCapsValid(const StatCaps& C)
{
	return C.Hp >= 0 && C.Strength >= 0 && C.Skill >= 0 && C.Speed >= 0 && C.Defense >= 0;
}

inline bool IsPromotionValid(const PromotionSpec& P)
{
	const StatBonus& B = P.Bonus;
	return P.MinLevel >= 1 && P.MinLevel <= MaxLevel && AreRatesValid(P.Growth) && AreCapsValid(P.Caps) && B.Hp >= 0
		&& B.Strength >= 0 && B.Skill >= 0 && B.Speed >= 0 && B.Defense >= 0 && B.Movement >= 0;
}

// Position-independent checks on a unit's own data: movement and Hp, the equipped weapon, level, experience,
// growth rates, caps, promotion, weapon training and the inventory.
inline bool IsUnitDataValid(const BattleUnit& Unit)
{
	if (Unit.Movement < 0 || Unit.Hp <= 0 || !IsWeaponConsistent(Unit.Weapon, Unit.WeaponUses) || Unit.Level < 1
		|| Unit.Level > MaxLevel || Unit.Experience < 0 || Unit.Experience >= ExperiencePerLevel || !AreRatesValid(Unit.Growth)
		|| !AreCapsValid(Unit.Caps) || Unit.Items.size() > MaxItemSlots || Unit.Promotions.size() > MaxPromotionOptions)
	{
		return false;
	}
	for (const PromotionSpec& Promotion : Unit.Promotions)
	{
		if (!IsPromotionValid(Promotion))
		{
			return false;
		}
	}
	for (const std::int32_t Points : Unit.WeaponPoints)
	{
		if (Points < 0 || Points > MaxWeaponPoints)
		{
			return false;
		}
	}
	for (const ItemSlot& Slot : Unit.Items)
	{
		if (!IsItemSlotValid(Slot))
		{
			return false;
		}
	}
	return true;
}

} // namespace commander::domain
