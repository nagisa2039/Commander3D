#pragma once

#include <algorithm>
#include <optional>
#include <utility>
#include <vector>

#include "Battle/BattleMap.h"
#include "Battle/BattleUnit.h"
#include "Battle/Combat.h"
#include "Battle/MovementRange.h"
#include "Battle/Progression.h"
#include "Battle/RandomSource.h"
#include "Battle/Shop.h"
#include "Battle/UnitValidation.h"
#include "Battle/VictoryCondition.h"

namespace commander::domain
{

// Event: a unit moved along Path (start and destination inclusive).
struct UnitMoved
{
	UnitId Unit;
	std::vector<GridCoord> Path;
};

// Event: a unit ended its action without doing anything else.
struct UnitWaited
{
	UnitId Unit;
};

// Event: a spare weapon from inventory slot Slot was equipped (the old weapon took its place in the slot,
// or the slot was freed if the hand was empty).
struct WeaponEquipped
{
	UnitId Unit;
	std::size_t Slot = 0;
};

// Event: an item was used. HpHealed is for potions, WeaponRepaired for repair kits.
struct ItemUsed
{
	UnitId Unit;
	ItemKind Kind = ItemKind::Potion;
	std::int32_t HpHealed = 0;
	bool WeaponRepaired = false;
};

// Event: a unit gave the item in its slot Slot to an adjacent ally.
struct ItemGiven
{
	UnitId Giver;
	UnitId Receiver;
	std::size_t Slot = 0; // the giver's slot the item came from
};

// Event: a unit discarded the item in its slot Slot.
struct ItemDropped
{
	UnitId Unit;
	std::size_t Slot = 0;
};

// Event: a unit healed an ally with a staff. A player healer also earns Experience (and may level up).
struct StaffUsed
{
	UnitId Healer;
	UnitId Target;
	std::int32_t HpHealed = 0;
	std::optional<ExperienceGained> Experience;
	std::optional<LevelUp> LevelUp;
};

// Event: a unit bought the shop's stock entry Entry for Price gold.
struct ItemBought
{
	UnitId Unit;
	std::size_t Entry = 0;
	std::int32_t Price = 0;
};

// Event: a unit sold the item in its slot Slot for Gold.
struct ItemSold
{
	UnitId Unit;
	std::size_t Slot = 0;
	std::int32_t Gold = 0;
};

// Event: a unit changed class into the chosen option (Name); Bonus is what it gained. UsedSeal: a promotion
// seal was spent.
struct UnitPromoted
{
	UnitId Unit;
	std::size_t Option = 0;
	std::string Name;
	StatBonus Bonus;
	bool UsedSeal = false;
};

// One blow within an attack. DefenderHp is the defender's Hp after the blow.
// WeaponBroke: the striker's weapon wore out with this blow (hit or miss).
// WeaponRankUp: the blow trained the striker's weapon type into a higher rank.
struct Strike
{
	UnitId Attacker;
	UnitId Defender;
	bool Hit = false;
	std::int32_t Damage = 0;
	std::int32_t DefenderHp = 0;
	bool WeaponBroke = false;
	bool WeaponRankUp = false;
};

// Event: an attack and its counter, in the order they happened, plus the units that were defeated,
// the experience surviving player units earned and the level-ups it caused.
struct AttackResolved
{
	std::vector<Strike> Strikes;
	std::vector<UnitId> Defeated;
	std::vector<ExperienceGained> Experience;
	std::vector<LevelUp> LevelUps;
	BattleOutcome Outcome = BattleOutcome::Ongoing; // The battle's outcome after this attack.
};

// Event: a new faction phase began. Turn is the turn number it belongs to.
// Outcome is the battle's outcome after the change (PlayerLost when the turn limit is exceeded).
struct PhaseChanged
{
	Faction Side = Faction::Player;
	std::int32_t Turn = 1;
	BattleOutcome Outcome = BattleOutcome::Ongoing;
};

// One map's state: terrain plus the units on it. Aggregate root.
class Battle
{
public:
	explicit Battle(BattleMap InMap, VictoryConditions InConditions = {})
		: Map(std::move(InMap)), Conditions(InConditions)
	{
	}

	const BattleMap& GetMap() const { return Map; }
	const std::vector<BattleUnit>& Units() const { return UnitList; }

	// Fails (false) on duplicate id, inconsistent unit data (see IsUnitDataValid: movement, Hp, weapon, level,
	// experience, growth, inventory), a cell outside the grid, impassable or occupied.
	bool AddUnit(const BattleUnit& Unit)
	{
		const TerrainSpec* Terrain = Map.TerrainAt(Unit.Position);
		if (Terrain == nullptr || !Terrain->Passable || !IsUnitDataValid(Unit) || FindUnit(Unit.Id) != nullptr
			|| UnitAt(Unit.Position) != nullptr)
		{
			return false;
		}
		UnitList.push_back(Unit);
		return true;
	}

	const std::vector<Shop>& Shops() const { return ShopList; }

	// Fails (false) if the cell is outside the grid or impassable, already has a shop, or the stock is empty or
	// holds an invalid item or one that is not for sale (Price must be > 0).
	bool AddShop(const Shop& NewShop)
	{
		const TerrainSpec* Terrain = Map.TerrainAt(NewShop.Position);
		if (Terrain == nullptr || !Terrain->Passable || FindShop(NewShop.Position) != nullptr || NewShop.Stock.empty())
		{
			return false;
		}
		for (const ItemSlot& Item : NewShop.Stock)
		{
			if (!IsItemSlotValid(Item) || Item.Price <= 0)
			{
				return false;
			}
		}
		ShopList.push_back(NewShop);
		return true;
	}

	// The player army's gold. Fails (false) for a negative amount.
	std::int32_t Gold() const { return GoldValue; }
	bool SetGold(std::int32_t Amount)
	{
		if (Amount < 0)
		{
			return false;
		}
		GoldValue = Amount;
		return true;
	}

	const BattleUnit* FindUnit(UnitId Id) const
	{
		for (const BattleUnit& U : UnitList)
		{
			if (U.Id == Id)
			{
				return &U;
			}
		}
		return nullptr;
	}

	const BattleUnit* UnitAt(const GridCoord& Coord) const
	{
		for (const BattleUnit& U : UnitList)
		{
			if (U.Position == Coord)
			{
				return &U;
			}
		}
		return nullptr;
	}

	// Ongoing until a defeat decides the battle (evaluated after each attack). Once decided,
	// Move/Wait/Attack are rejected and EndPhase changes nothing.
	BattleOutcome Outcome() const { return OutcomeValue; }
	Faction CurrentFaction() const { return CurrentFactionValue; }
	std::int32_t TurnNumber() const { return Turn; }

	// Returns nullopt (state unchanged) if the unit is unknown, is not of the current phase's faction,
	// has already moved or acted, or Destination is not a cell it can stop on.
	// A move to the unit's own cell is valid (path of one cell).
	std::optional<UnitMoved> MoveUnit(UnitId Id, const GridCoord& Destination)
	{
		BattleUnit* Unit = FindActionable(Id);
		if (Unit == nullptr || Unit->HasMoved)
		{
			return std::nullopt;
		}
		const MovementRange Range = CalculateMovementRange(Map, UnitList, *Unit);
		if (!Range.CanStopAt(Destination))
		{
			return std::nullopt;
		}
		UnitMoved Event{Id, Range.PathTo(Destination)};
		Unit->Position = Destination;
		Unit->HasMoved = true;
		return Event;
	}

	// Ends the unit's action. Nullopt if unknown, not the current faction's, or already acted.
	std::optional<UnitWaited> WaitUnit(UnitId Id)
	{
		BattleUnit* Unit = FindActionable(Id);
		if (Unit == nullptr)
		{
			return std::nullopt;
		}
		Unit->HasActed = true;
		return UnitWaited{Id};
	}

	// Swaps the equipped weapon with the weapon in inventory slot Slot. Free: it does not use up the action and
	// may be done before or after moving. Nullopt (state unchanged) if the unit is not actionable, the slot does
	// not exist or does not hold a weapon, or the unit lacks the rank to wield it.
	std::optional<WeaponEquipped> EquipWeapon(UnitId Id, std::size_t Slot)
	{
		BattleUnit* Unit = FindActionable(Id);
		if (Unit == nullptr || Slot >= Unit->Items.size() || Unit->Items[Slot].Kind != ItemKind::Weapon
			|| !CanWield(*Unit, Unit->Items[Slot].Weapon))
		{
			return std::nullopt;
		}
		ItemSlot& Item = Unit->Items[Slot];
		if (Unit->Weapon.MaxRange > 0)
		{
			std::swap(Unit->Weapon, Item.Weapon);
			std::swap(Unit->WeaponUses, Item.Uses);
		}
		else
		{
			Unit->Weapon = Item.Weapon;
			Unit->WeaponUses = Item.Uses;
			Unit->Items.erase(Unit->Items.begin() + static_cast<std::ptrdiff_t>(Slot));
		}
		return WeaponEquipped{Id, Slot};
	}

	// Uses the potion or repair kit in slot Slot; this ends the unit's action. Nullopt (state unchanged) if the
	// unit is not actionable, the slot is missing or holds a weapon, a potion is used at full Hp, or a repair
	// kit is used on a weapon that cannot break or is already at full durability. Emptied items are removed.
	std::optional<ItemUsed> UseItem(UnitId Id, std::size_t Slot)
	{
		BattleUnit* Unit = FindActionable(Id);
		if (Unit == nullptr || Slot >= Unit->Items.size())
		{
			return std::nullopt;
		}
		ItemSlot& Item = Unit->Items[Slot];
		ItemUsed Event;
		Event.Unit = Id;
		Event.Kind = Item.Kind;
		switch (Item.Kind)
		{
		case ItemKind::Potion:
			if (Unit->Hp >= Unit->MaxHp)
			{
				return std::nullopt;
			}
			Event.HpHealed = std::min(Item.Amount, Unit->MaxHp - Unit->Hp);
			Unit->Hp += Event.HpHealed;
			break;
		case ItemKind::RepairKit:
			if (Unit->Weapon.Durability <= 0 || Unit->WeaponUses >= Unit->Weapon.Durability)
			{
				return std::nullopt;
			}
			Unit->WeaponUses = Unit->Weapon.Durability;
			Event.WeaponRepaired = true;
			break;
		case ItemKind::Weapon:
		case ItemKind::Staff:         // used on an ally with UseStaff
		case ItemKind::PromotionSeal: // spent by PromoteUnit
			return std::nullopt;
		}
		if (--Item.Uses <= 0)
		{
			Unit->Items.erase(Unit->Items.begin() + static_cast<std::ptrdiff_t>(Slot));
		}
		Unit->HasMoved = true;
		Unit->HasActed = true;
		return Event;
	}

	// Hands the item in the giver's slot Slot to an adjacent (distance 1) ally with room in its pack. Uses up the
	// giver's action. Nullopt (state unchanged) if the giver is not actionable, the receiver is unknown, the
	// giver itself, on the other side or not adjacent, the slot is missing or the receiver's pack is full.
	std::optional<ItemGiven> GiveItem(UnitId GiverId, UnitId ReceiverId, std::size_t Slot)
	{
		BattleUnit* Giver = FindActionable(GiverId);
		BattleUnit* Receiver = FindMutable(ReceiverId);
		if (Giver == nullptr || Receiver == nullptr || Giver == Receiver || Receiver->Side != Giver->Side
			|| ManhattanDistance(Giver->Position, Receiver->Position) != 1 || Slot >= Giver->Items.size()
			|| Receiver->Items.size() >= MaxItemSlots)
		{
			return std::nullopt;
		}
		Receiver->Items.push_back(Giver->Items[Slot]);
		Giver->Items.erase(Giver->Items.begin() + static_cast<std::ptrdiff_t>(Slot));
		Giver->HasMoved = true;
		Giver->HasActed = true;
		return ItemGiven{GiverId, ReceiverId, Slot};
	}

	// Discards the item in slot Slot. Free (does not use up the action). Nullopt if the unit is not actionable
	// or the slot is missing.
	std::optional<ItemDropped> DropItem(UnitId Id, std::size_t Slot)
	{
		BattleUnit* Unit = FindActionable(Id);
		if (Unit == nullptr || Slot >= Unit->Items.size())
		{
			return std::nullopt;
		}
		Unit->Items.erase(Unit->Items.begin() + static_cast<std::ptrdiff_t>(Slot));
		return ItemDropped{Id, Slot};
	}

	// Heals an ally within the staff's Range (distance 1..Range, not the healer itself) that is missing Hp,
	// using the staff in slot Slot; this ends the healer's action. Spent staffs are removed. Nullopt (state
	// unchanged) if the healer is not actionable, the slot is not a staff, or the target is unknown, itself,
	// on the other side, out of range or already at full Hp. A player healer earns CalculateStaffExperience
	// (growth rolls come from Rng); enemy healers earn nothing.
	std::optional<StaffUsed> UseStaff(UnitId HealerId, UnitId TargetId, std::size_t Slot, IRandomSource& Rng)
	{
		BattleUnit* Healer = FindActionable(HealerId);
		BattleUnit* Target = FindMutable(TargetId);
		if (Healer == nullptr || Target == nullptr || Healer == Target || Target->Side != Healer->Side
			|| Slot >= Healer->Items.size() || Healer->Items[Slot].Kind != ItemKind::Staff || Target->Hp >= Target->MaxHp)
		{
			return std::nullopt;
		}
		ItemSlot& Staff = Healer->Items[Slot];
		if (ManhattanDistance(Healer->Position, Target->Position) > Staff.Range)
		{
			return std::nullopt;
		}
		StaffUsed Event;
		Event.Healer = HealerId;
		Event.Target = TargetId;
		Event.HpHealed = std::min(Staff.Amount, Target->MaxHp - Target->Hp);
		Target->Hp += Event.HpHealed;
		if (--Staff.Uses <= 0)
		{
			Healer->Items.erase(Healer->Items.begin() + static_cast<std::ptrdiff_t>(Slot));
		}
		Healer->HasMoved = true;
		Healer->HasActed = true;
		if (Healer->Side == Faction::Player && Healer->Level < MaxLevel)
		{
			const std::int32_t Amount = CalculateStaffExperience(Event.HpHealed);
			Event.Experience = ExperienceGained{HealerId, Amount};
			LevelUp Gained;
			if (AddExperience(*Healer, Amount, Rng, Gained))
			{
				Event.LevelUp = Gained;
			}
		}
		return Event;
	}

	// A player unit standing on a shop buys stock entry Entry with the army's gold; the item goes into its pack.
	// Free (does not use up the action). Nullopt (state unchanged) if the unit is not actionable or not a player
	// unit, there is no shop under it, the entry does not exist, gold is short or the pack is full.
	std::optional<ItemBought> BuyItem(UnitId Id, std::size_t Entry)
	{
		BattleUnit* Unit = FindActionable(Id);
		const Shop* Here = Unit != nullptr ? FindShop(Unit->Position) : nullptr;
		if (Unit == nullptr || Unit->Side != Faction::Player || Here == nullptr || Entry >= Here->Stock.size()
			|| Unit->Items.size() >= MaxItemSlots || GoldValue < Here->Stock[Entry].Price)
		{
			return std::nullopt;
		}
		const ItemSlot Item = Here->Stock[Entry];
		GoldValue -= Item.Price;
		Unit->Items.push_back(Item);
		return ItemBought{Id, Entry, Item.Price};
	}

	// A player unit standing on a shop sells the item in its slot Slot for half its Price (rounded down).
	// Free. Nullopt (state unchanged) if the unit is not actionable or not a player unit, there is no shop under
	// it, the slot is missing or the item has no price.
	std::optional<ItemSold> SellItem(UnitId Id, std::size_t Slot)
	{
		BattleUnit* Unit = FindActionable(Id);
		if (Unit == nullptr || Unit->Side != Faction::Player || FindShop(Unit->Position) == nullptr || Slot >= Unit->Items.size()
			|| Unit->Items[Slot].Price / 2 <= 0)
		{
			return std::nullopt;
		}
		const std::int32_t Earned = Unit->Items[Slot].Price / 2;
		GoldValue += Earned;
		Unit->Items.erase(Unit->Items.begin() + static_cast<std::ptrdiff_t>(Slot));
		return ItemSold{Id, Slot, Earned};
	}

	// Changes class into the unit's promotion option Option: adds its stat bonuses (and movement), resets Level
	// to 1 and Experience to 0, replaces Growth and Caps and removes all remaining options (a unit promotes once).
	// Normally the unit must have reached the option's MinLevel. With SealSlot, the item in that slot must be a
	// promotion seal and the unit only needs the seal's level (Amount); one charge is spent. Ends the unit's
	// action. Nullopt (state unchanged) if the unit is not actionable, the option or seal slot does not exist, or
	// the level requirement is not met.
	std::optional<UnitPromoted> PromoteUnit(UnitId Id, std::size_t Option, std::optional<std::size_t> SealSlot = std::nullopt)
	{
		BattleUnit* Unit = FindActionable(Id);
		if (Unit == nullptr || Option >= Unit->Promotions.size())
		{
			return std::nullopt;
		}
		std::int32_t RequiredLevel = Unit->Promotions[Option].MinLevel;
		if (SealSlot.has_value())
		{
			if (*SealSlot >= Unit->Items.size() || Unit->Items[*SealSlot].Kind != ItemKind::PromotionSeal)
			{
				return std::nullopt;
			}
			RequiredLevel = Unit->Items[*SealSlot].Amount;
		}
		if (Unit->Level < RequiredLevel)
		{
			return std::nullopt;
		}

		const PromotionSpec Promotion = Unit->Promotions[Option];
		if (SealSlot.has_value() && --Unit->Items[*SealSlot].Uses <= 0)
		{
			Unit->Items.erase(Unit->Items.begin() + static_cast<std::ptrdiff_t>(*SealSlot));
		}
		Unit->MaxHp += Promotion.Bonus.Hp;
		Unit->Hp += Promotion.Bonus.Hp;
		Unit->Strength += Promotion.Bonus.Strength;
		Unit->Skill += Promotion.Bonus.Skill;
		Unit->Speed += Promotion.Bonus.Speed;
		Unit->Defense += Promotion.Bonus.Defense;
		Unit->Movement += Promotion.Bonus.Movement;
		Unit->Level = 1;
		Unit->Experience = 0;
		Unit->Growth = Promotion.Growth;
		Unit->Caps = Promotion.Caps;
		Unit->Promotions.clear();
		Unit->HasMoved = true;
		Unit->HasActed = true;
		UnitPromoted Event;
		Event.Unit = Id;
		Event.Option = Option;
		Event.Name = Promotion.Name;
		Event.Bonus = Promotion.Bonus;
		Event.UsedSeal = SealSlot.has_value();
		return Event;
	}

	// The attacker strikes first; if the target survives and the attacker is within the target's weapon
	// range, the target strikes back. Units reaching 0 Hp are removed. The attacker's action ends.
	// Nullopt (state unchanged) if either unit is unknown, the attacker is not actionable this phase,
	// the target is on the same side, or the target is outside the attacker's weapon range.
	std::optional<AttackResolved> AttackUnit(UnitId AttackerId, UnitId TargetId, IRandomSource& Rng)
	{
		BattleUnit* Attacker = FindActionable(AttackerId);
		BattleUnit* Target = FindMutable(TargetId);
		if (Attacker == nullptr || Target == nullptr || Attacker->Side == Target->Side)
		{
			return std::nullopt;
		}
		const std::int32_t Distance = ManhattanDistance(Attacker->Position, Target->Position);
		if (!WeaponReaches(*Attacker, Distance))
		{
			return std::nullopt;
		}

		AttackResolved Result;
		Result.Strikes.push_back(Resolve(*Attacker, *Target, Rng));
		if (Target->Hp > 0 && WeaponReaches(*Target, Distance))
		{
			Result.Strikes.push_back(Resolve(*Target, *Attacker, Rng));
		}
		Attacker->HasMoved = true;
		Attacker->HasActed = true;

		// Experience goes to surviving player units that took part: the attacker, and the target if it struck back.
		const auto Award = [&Rng, &Result](BattleUnit& Self, const BattleUnit& Opponent, bool Killed) {
			if (Self.Side != Faction::Player || Self.Hp <= 0 || Self.Level >= MaxLevel)
			{
				return;
			}
			const std::int32_t Amount = CalculateExperience(Self, Opponent, Killed);
			Result.Experience.push_back(ExperienceGained{Self.Id, Amount});
			LevelUp Gained;
			if (AddExperience(Self, Amount, Rng, Gained))
			{
				Result.LevelUps.push_back(Gained);
			}
		};
		Award(*Attacker, *Target, Target->Hp <= 0);
		if (Result.Strikes.size() > 1)
		{
			Award(*Target, *Attacker, Attacker->Hp <= 0);
		}

		for (const Strike& S : Result.Strikes)
		{
			if (S.DefenderHp <= 0)
			{
				Result.Defeated.push_back(S.Defender);
			}
		}
		std::erase_if(UnitList, [](const BattleUnit& U) { return U.Hp <= 0; });
		OutcomeValue = EvaluateOutcome(UnitList, Conditions, Turn);
		Result.Outcome = OutcomeValue;
		return Result;
	}

	// Hands the phase to the other faction and makes all units ready. The turn number advances
	// when control returns to the player.
	PhaseChanged EndPhase()
	{
		if (OutcomeValue != BattleOutcome::Ongoing)
		{
			return PhaseChanged{CurrentFactionValue, Turn, OutcomeValue};
		}
		CurrentFactionValue = CurrentFactionValue == Faction::Player ? Faction::Enemy : Faction::Player;
		if (CurrentFactionValue == Faction::Player)
		{
			++Turn;
		}
		for (BattleUnit& U : UnitList)
		{
			U.HasMoved = false;
			U.HasActed = false;
		}
		OutcomeValue = EvaluateOutcome(UnitList, Conditions, Turn);
		return PhaseChanged{CurrentFactionValue, Turn, OutcomeValue};
	}

private:
	const Shop* FindShop(const GridCoord& Coord) const
	{
		for (const Shop& S : ShopList)
		{
			if (S.Position == Coord)
			{
				return &S;
			}
		}
		return nullptr;
	}

	// One blow from From to To. Uses up one weapon use (hit or miss) when the weapon has a Durability.
	static Strike Resolve(BattleUnit& From, BattleUnit& To, IRandomSource& Rng)
	{
		Strike S;
		S.Attacker = From.Id;
		S.Defender = To.Id;
		S.Hit = Rng.NextPercent() < CalculateHitRate(From, To);
		if (S.Hit)
		{
			S.Damage = CalculateDamage(From, To);
			To.Hp = std::max(0, To.Hp - S.Damage);
		}
		S.DefenderHp = To.Hp;
		if (From.Weapon.Durability > 0)
		{
			--From.WeaponUses;
			S.WeaponBroke = From.WeaponUses <= 0;
		}
		// Every strike trains the player's weapon type (enemies and untyped weapons do not train).
		if (From.Side == Faction::Player && From.Weapon.Type > 0)
		{
			std::int32_t& Points = From.WeaponPoints[static_cast<std::size_t>(From.Weapon.Type)];
			const std::int32_t RankBefore = WeaponRankOf(Points);
			Points = std::min(Points + 1, MaxWeaponPoints);
			S.WeaponRankUp = WeaponRankOf(Points) > RankBefore;
		}
		return S;
	}

	BattleUnit* FindActionable(UnitId Id)
	{
		BattleUnit* Unit = FindMutable(Id);
		if (OutcomeValue != BattleOutcome::Ongoing)
		{
			return nullptr;
		}
		return (Unit != nullptr && Unit->Side == CurrentFactionValue && !Unit->HasActed) ? Unit : nullptr;
	}

	BattleUnit* FindMutable(UnitId Id)
	{
		for (BattleUnit& U : UnitList)
		{
			if (U.Id == Id)
			{
				return &U;
			}
		}
		return nullptr;
	}

	BattleMap Map;
	VictoryConditions Conditions;
	std::vector<BattleUnit> UnitList;
	std::vector<Shop> ShopList;
	std::int32_t GoldValue = 0;
	Faction CurrentFactionValue = Faction::Player;
	std::int32_t Turn = 1;
	BattleOutcome OutcomeValue = BattleOutcome::Ongoing;
};

} // namespace commander::domain
