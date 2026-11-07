#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

#include "Battle/BattleUnit.h"

namespace commander::domain
{

enum class BattleOutcome : std::uint8_t
{
	Ongoing,
	PlayerWon,
	PlayerLost,
};

// Which conditions end the battle. The player always loses when no player unit remains.
struct VictoryConditions
{
	bool RoutEnemies = true;     // Win when no enemy remains.
	bool DefeatBoss = false;     // Win when no enemy-side boss (IsBoss) remains. A map using this must have a boss.
	std::int32_t TurnLimit = 0;  // Lose once the turn number exceeds this. 0 = no limit.
};

// Precedence: no player units -> lost; a win condition met -> won; turn limit exceeded -> lost; else ongoing.
// A win on the last allowed turn therefore counts as a win.
inline BattleOutcome EvaluateOutcome(const std::vector<BattleUnit>& Units, const VictoryConditions& Conditions = {},
	std::int32_t Turn = 1)
{
	const auto Any = [&Units](auto Predicate) { return std::any_of(Units.begin(), Units.end(), Predicate); };
	if (!Any([](const BattleUnit& U) { return U.Side == Faction::Player; }))
	{
		return BattleOutcome::PlayerLost;
	}
	if (Conditions.RoutEnemies && !Any([](const BattleUnit& U) { return U.Side == Faction::Enemy; }))
	{
		return BattleOutcome::PlayerWon;
	}
	if (Conditions.DefeatBoss && !Any([](const BattleUnit& U) { return U.Side == Faction::Enemy && U.IsBoss; }))
	{
		return BattleOutcome::PlayerWon;
	}
	if (Conditions.TurnLimit > 0 && Turn > Conditions.TurnLimit)
	{
		return BattleOutcome::PlayerLost;
	}
	return BattleOutcome::Ongoing;
}

} // namespace commander::domain
