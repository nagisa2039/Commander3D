#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "Battle/BattleUnit.h"
#include "Battle/GridCoord.h"

namespace commander::domain
{

// A shop on a map cell. A player unit standing on Position can buy from Stock (the stock never runs out;
// each entry's Price is its buying price) and sell inventory items for half their Price.
struct Shop
{
	GridCoord Position;
	std::vector<ItemSlot> Stock;
};

// The buying rule, shared by the battle (Battle::BuyItem) and the pre-battle preparation: the unit pays Stock[Entry]'s
// Price from Gold and gets the item. Returns the price paid, or nullopt (nothing changed) if the entry does not
// exist, the pack is full or Gold is short.
inline std::optional<std::int32_t> BuyFromStock(BattleUnit& Unit, std::int32_t& Gold, const std::vector<ItemSlot>& Stock, std::size_t Entry)
{
	if (Entry >= Stock.size() || Unit.Items.size() >= MaxItemSlots || Gold < Stock[Entry].Price)
	{
		return std::nullopt;
	}
	const ItemSlot Item = Stock[Entry];
	Gold -= Item.Price;
	Unit.Items.push_back(Item);
	return Item.Price;
}

// The selling rule: the item in the unit's slot Slot is sold for half its Price (rounded down) and added to Gold.
// Returns the gold earned, or nullopt (nothing changed) if the slot is missing or the item sells for nothing.
inline std::optional<std::int32_t> SellFromPack(BattleUnit& Unit, std::int32_t& Gold, std::size_t Slot)
{
	if (Slot >= Unit.Items.size() || Unit.Items[Slot].Price / 2 <= 0)
	{
		return std::nullopt;
	}
	const std::int32_t Earned = Unit.Items[Slot].Price / 2;
	Gold += Earned;
	Unit.Items.erase(Unit.Items.begin() + static_cast<std::ptrdiff_t>(Slot));
	return Earned;
}

} // namespace commander::domain
