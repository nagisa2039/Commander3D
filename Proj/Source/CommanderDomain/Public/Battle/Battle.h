#pragma once

#include <algorithm>
#include <optional>
#include <utility>
#include <vector>

#include "Battle/BattleMap.h"
#include "Battle/BattleUnit.h"
#include "Battle/Combat.h"
#include "Battle/MovementRange.h"
#include "Battle/RandomSource.h"
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

// One blow within an attack. DefenderHp is the defender's Hp after the blow.
struct Strike
{
	UnitId Attacker;
	UnitId Defender;
	bool Hit = false;
	std::int32_t Damage = 0;
	std::int32_t DefenderHp = 0;
};

// Event: an attack and its counter, in the order they happened, plus the units that were defeated.
struct AttackResolved
{
	std::vector<Strike> Strikes;
	std::vector<UnitId> Defeated;
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

	// Fails (false) on duplicate id, negative movement, non-positive Hp, a cell outside the grid,
	// impassable or occupied.
	bool AddUnit(const BattleUnit& Unit)
	{
		const TerrainSpec* Terrain = Map.TerrainAt(Unit.Position);
		if (Terrain == nullptr || !Terrain->Passable || Unit.Movement < 0 || Unit.Hp <= 0 || FindUnit(Unit.Id) != nullptr
			|| UnitAt(Unit.Position) != nullptr)
		{
			return false;
		}
		UnitList.push_back(Unit);
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
	static Strike Resolve(const BattleUnit& From, BattleUnit& To, IRandomSource& Rng)
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
	Faction CurrentFactionValue = Faction::Player;
	std::int32_t Turn = 1;
	BattleOutcome OutcomeValue = BattleOutcome::Ongoing;
};

} // namespace commander::domain
