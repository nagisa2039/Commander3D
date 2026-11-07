#pragma once

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

} // namespace commander::domain
