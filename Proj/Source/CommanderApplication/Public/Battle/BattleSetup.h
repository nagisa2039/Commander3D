#pragma once

#include <algorithm>
#include <utility>
#include <vector>

#include "Battle/Battle.h"
#include "Common/Result.h"
#include "Map/MapLoader.h"

namespace commander::application
{

// Everything needed to start a battle: the map, the units placed on it and the victory conditions.
struct BattleSetup
{
	MapData Map;
	std::vector<domain::BattleUnit> Units;
	domain::VictoryConditions Conditions;
	std::int32_t Gold = 0;           // the player army's starting gold
	std::vector<domain::Shop> Shops;
};

enum class BattleSetupError
{
	InvalidMap,
	InvalidUnit,   // duplicate id, bad position (outside, wall, occupied) or inconsistent unit data
	InvalidShop,   // negative gold, or a shop on a bad cell / on an occupied-by-another-shop cell / with bad or unpriced stock
	NoPlayerUnit,
	NoEnemyUnit,
	NoBoss,        // DefeatBoss is on but no enemy-side unit has IsBoss
};

// Validates the setup and builds a Battle at turn 1, player phase. Checks run in the order of the error values.
inline Result<domain::Battle, BattleSetupError> StartBattle(const BattleSetup& Setup)
{
	using Outcome = Result<domain::Battle, BattleSetupError>;

	auto Map = LoadMap(Setup.Map);
	if (!Map.IsOk())
	{
		return Outcome::Err(BattleSetupError::InvalidMap);
	}

	domain::Battle B(Map.Value(), Setup.Conditions);
	for (const domain::BattleUnit& Unit : Setup.Units)
	{
		if (!B.AddUnit(Unit))
		{
			return Outcome::Err(BattleSetupError::InvalidUnit);
		}
	}

	if (!B.SetGold(Setup.Gold))
	{
		return Outcome::Err(BattleSetupError::InvalidShop);
	}
	for (const domain::Shop& Shop : Setup.Shops)
	{
		if (!B.AddShop(Shop))
		{
			return Outcome::Err(BattleSetupError::InvalidShop);
		}
	}

	const auto& Units = B.Units();
	const auto Has = [&Units](auto Predicate) { return std::any_of(Units.begin(), Units.end(), Predicate); };
	if (!Has([](const domain::BattleUnit& U) { return U.Side == domain::Faction::Player; }))
	{
		return Outcome::Err(BattleSetupError::NoPlayerUnit);
	}
	if (!Has([](const domain::BattleUnit& U) { return U.Side == domain::Faction::Enemy; }))
	{
		return Outcome::Err(BattleSetupError::NoEnemyUnit);
	}
	if (Setup.Conditions.DefeatBoss
		&& !Has([](const domain::BattleUnit& U) { return U.Side == domain::Faction::Enemy && U.IsBoss; }))
	{
		return Outcome::Err(BattleSetupError::NoBoss);
	}
	return Outcome::Ok(std::move(B));
}

} // namespace commander::application
