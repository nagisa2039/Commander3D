#pragma once

#include <cstdint>
#include <optional>

#include "Battle/BattleUnit.h"
#include "Battle/GridCoord.h"

namespace commander::domain
{

// A treasure chest on a map cell. A player unit standing on Position can open it (using up its action) and gets
// either Item (into its pack) or Gold (into the army's gold); a chest holds exactly one of the two. It is gone once opened.
struct Chest
{
	GridCoord Position;
	std::optional<ItemSlot> Item;
	std::int32_t Gold = 0;
};

} // namespace commander::domain
