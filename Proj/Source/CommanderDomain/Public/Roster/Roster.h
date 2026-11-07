#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "Battle/Battle.h"
#include "Battle/BattleUnit.h"
#include "Battle/UnitValidation.h"

namespace commander::domain
{

enum class Difficulty : std::uint8_t
{
	Casual,  // units that fall in battle come back next chapter
	Classic, // units that fall in battle are lost for good
};

// The player's persistent army between battles: the characters (with level, experience, items, training) and gold.
// Members are stored as BattleUnits of the player side; their map state (position, HasMoved, HasActed) is not kept.
class Roster
{
public:
	const std::vector<BattleUnit>& Members() const { return MemberList; }
	std::int32_t Gold() const { return GoldValue; }

	const BattleUnit* Find(UnitId Id) const
	{
		const auto It = FindIt(Id);
		return It == MemberList.end() ? nullptr : &*It;
	}

	// False (unchanged) if the unit is not a player, its data is invalid or the id is already in the roster.
	bool Add(const BattleUnit& Unit)
	{
		if (Unit.Side != Faction::Player || !IsUnitDataValid(Unit) || Find(Unit.Id) != nullptr)
		{
			return false;
		}
		MemberList.push_back(Unit);
		Normalize(MemberList.back());
		return true;
	}

	bool SetGold(std::int32_t Amount)
	{
		if (Amount < 0)
		{
			return false;
		}
		GoldValue = Amount;
		return true;
	}

	// A fresh battle unit for the member at Position: fully healed, nothing moved or acted. Nullopt if unknown.
	std::optional<BattleUnit> Deploy(UnitId Id, const GridCoord& Position) const
	{
		const BattleUnit* Member = Find(Id);
		if (Member == nullptr)
		{
			return std::nullopt;
		}
		BattleUnit Unit = *Member;
		Unit.Position = Position;
		return Unit;
	}

	// Moves the item in the giver's slot to the receiver's pack. False (unchanged) if either is unknown, they are
	// the same member, the slot is missing or the receiver's pack is full.
	bool TransferItem(UnitId GiverId, UnitId ReceiverId, std::size_t Slot)
	{
		const auto Giver = FindIt(GiverId);
		const auto Receiver = FindIt(ReceiverId);
		if (Giver == MemberList.end() || Receiver == MemberList.end() || Giver == Receiver || Slot >= Giver->Items.size()
			|| Receiver->Items.size() >= MaxItemSlots)
		{
			return false;
		}
		Receiver->Items.push_back(Giver->Items[Slot]);
		Giver->Items.erase(Giver->Items.begin() + static_cast<std::ptrdiff_t>(Slot));
		return true;
	}

	// The member buys Stock[Entry] with the roster's gold (see BuyFromStock). False (unchanged) if the member or entry
	// is missing, the pack is full or gold is short.
	bool BuyItem(UnitId Id, const std::vector<ItemSlot>& Stock, std::size_t Entry)
	{
		const auto It = FindIt(Id);
		return It != MemberList.end() && BuyFromStock(*It, GoldValue, Stock, Entry).has_value();
	}

	// The member sells the item in the slot for half its price (see SellFromPack). False (unchanged) if the member or
	// slot is missing or the item has no price.
	bool SellItem(UnitId Id, std::size_t Slot)
	{
		const auto It = FindIt(Id);
		return It != MemberList.end() && SellFromPack(*It, GoldValue, Slot).has_value();
	}

	// Throws away the item in the slot. False if the member or slot is missing.
	bool DiscardItem(UnitId Id, std::size_t Slot)
	{
		const auto It = FindIt(Id);
		if (It == MemberList.end() || Slot >= It->Items.size())
		{
			return false;
		}
		It->Items.erase(It->Items.begin() + static_cast<std::ptrdiff_t>(Slot));
		return true;
	}

	// Equips the weapon in the pack slot (see EquipFromPack). False (unchanged) if the member or slot is missing,
	// the slot is not a weapon, or the member lacks the rank to wield it.
	bool EquipWeapon(UnitId Id, std::size_t Slot)
	{
		const auto It = FindIt(Id);
		return It != MemberList.end() && EquipFromPack(*It, Slot);
	}

	// Carries a finished battle over: the gold, and every deployed member still standing in Finished (as it ended,
	// healed). A deployed member missing from Finished fell: it is removed on Classic and stays as it was before
	// the battle on Casual. Members that were not deployed are untouched.
	void ApplyBattleResult(const Battle& Finished, const std::vector<UnitId>& Deployed, Difficulty Mode)
	{
		GoldValue = Finished.Gold();
		for (const UnitId Id : Deployed)
		{
			const auto Member = FindIt(Id);
			if (Member == MemberList.end())
			{
				continue;
			}
			const auto& Units = Finished.Units();
			const auto Survivor = std::find_if(Units.begin(), Units.end(), [Id](const BattleUnit& U) {
				return U.Id == Id && U.Side == Faction::Player;
			});
			if (Survivor != Units.end())
			{
				*Member = *Survivor;
				Normalize(*Member);
			}
			else if (Mode == Difficulty::Classic)
			{
				MemberList.erase(Member);
			}
		}
	}

private:
	static void Normalize(BattleUnit& Unit)
	{
		Unit.Hp = Unit.MaxHp;
		Unit.Position = {};
		Unit.HasMoved = false;
		Unit.HasActed = false;
	}

	std::vector<BattleUnit>::iterator FindIt(UnitId Id)
	{
		return std::find_if(MemberList.begin(), MemberList.end(), [Id](const BattleUnit& U) { return U.Id == Id; });
	}
	std::vector<BattleUnit>::const_iterator FindIt(UnitId Id) const
	{
		return std::find_if(MemberList.begin(), MemberList.end(), [Id](const BattleUnit& U) { return U.Id == Id; });
	}

	std::vector<BattleUnit> MemberList;
	std::int32_t GoldValue = 0;
};

} // namespace commander::domain
