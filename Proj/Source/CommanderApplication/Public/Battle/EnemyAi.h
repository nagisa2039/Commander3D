#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <variant>
#include <vector>

#include "Battle/Battle.h"
#include "Battle/BattleSession.h"

namespace commander::application
{

namespace enemy_ai_detail
{

struct AttackPlan
{
	domain::GridCoord Cell;
	domain::UnitId Target;
	bool Kills = false;
	std::int32_t Damage = 0;
	std::int32_t Cost = 0;
};

// Kill first, then more damage, then the cheaper move, then the lower target id.
inline bool IsBetter(const AttackPlan& A, const AttackPlan& B)
{
	if (A.Kills != B.Kills)
	{
		return A.Kills;
	}
	if (A.Damage != B.Damage)
	{
		return A.Damage > B.Damage;
	}
	if (A.Cost != B.Cost)
	{
		return A.Cost < B.Cost;
	}
	return A.Target.Value < B.Target.Value;
}

// Cells the unit may end on, with their move cost. Without CanMove only its own cell.
inline std::vector<std::pair<domain::GridCoord, std::int32_t>> CandidateCells(const domain::Battle& B, const domain::BattleUnit& Unit, bool CanMove)
{
	std::vector<std::pair<domain::GridCoord, std::int32_t>> Cells;
	if (!CanMove)
	{
		Cells.push_back({Unit.Position, 0});
		return Cells;
	}
	const domain::MovementRange Range = domain::CalculateMovementRange(B.GetMap(), B.Units(), Unit);
	for (const domain::GridCoord& Cell : Range.StoppableCells())
	{
		Cells.push_back({Cell, Range.CostTo(Cell)});
	}
	return Cells;
}

inline std::optional<AttackPlan> BestAttack(const domain::Battle& B, const domain::BattleUnit& Unit, bool CanMove)
{
	std::optional<AttackPlan> Best;
	for (const auto& [Cell, Cost] : CandidateCells(B, Unit, CanMove))
	{
		for (const domain::BattleUnit& Target : B.Units())
		{
			if (Target.Side != domain::Faction::Player
				|| !domain::WeaponReaches(Unit, domain::ManhattanDistance(Cell, Target.Position)))
			{
				continue;
			}
			AttackPlan Plan;
			Plan.Cell = Cell;
			Plan.Target = Target.Id;
			Plan.Damage = domain::CalculateDamage(Unit, Target);
			Plan.Kills = Plan.Damage >= Target.Hp;
			Plan.Cost = Cost;
			if (!Best.has_value() || IsBetter(Plan, *Best))
			{
				Best = Plan;
			}
		}
	}
	return Best;
}

inline constexpr std::int32_t Unreachable = INT32_MAX;

// Terrain-only path cost from each player unit's cell to every cell (units do not block, so a crowd of
// allies cannot make a target look unreachable). One field per player unit.
inline std::vector<domain::MovementRange> PlayerPathCostFields(const domain::Battle& B)
{
	constexpr std::int32_t Unlimited = 1 << 20; // far above any real map's path cost
	std::vector<domain::MovementRange> Fields;
	for (const domain::BattleUnit& Other : B.Units())
	{
		if (Other.Side == domain::Faction::Player)
		{
			domain::BattleUnit Probe = Other;
			Probe.Movement = Unlimited;
			Fields.push_back(domain::CalculateMovementRange(B.GetMap(), {}, Probe));
		}
	}
	return Fields;
}

// The reachable cell with the shortest terrain path (movement-cost weighted) to any player unit; ties go to
// the cheaper move. The unit's own cell when nothing is closer. Player units that cannot be reached by any
// path (walled off) are ignored.
inline domain::GridCoord ApproachCell(const domain::Battle& B, const domain::BattleUnit& Unit)
{
	const std::vector<domain::MovementRange> Fields = PlayerPathCostFields(B);
	const auto DistanceToPlayers = [&Fields](const domain::GridCoord& Cell) {
		std::int32_t Nearest = Unreachable;
		for (const domain::MovementRange& Field : Fields)
		{
			const std::int32_t D = Field.CostTo(Cell);
			if (D >= 0 && D < Nearest)
			{
				Nearest = D;
			}
		}
		return Nearest;
	};

	domain::GridCoord BestCell = Unit.Position;
	std::int32_t BestDistance = DistanceToPlayers(Unit.Position);
	std::int32_t BestCost = 0;
	for (const auto& [Cell, Cost] : CandidateCells(B, Unit, true))
	{
		const std::int32_t D = DistanceToPlayers(Cell);
		if (D == Unreachable)
		{
			continue;
		}
		if (D < BestDistance || (D == BestDistance && Cost < BestCost))
		{
			BestCell = Cell;
			BestDistance = D;
			BestCost = Cost;
		}
	}
	return BestCell;
}

} // namespace enemy_ai_detail

// The next Command the enemy side wants to issue, or nullopt when it is not the enemy phase or the battle is over.
// Enemy units act in list order: attack the best target (moving into range if needed), else approach the
// nearest player unit, else wait. When every enemy unit has acted the answer is EndPhaseCommand.
// It goes through the same Commands as the player, and only reads the Battle.
inline std::optional<BattleCommand> DecideNext(const domain::Battle& B)
{
	using namespace enemy_ai_detail;

	if (B.Outcome() != domain::BattleOutcome::Ongoing || B.CurrentFaction() != domain::Faction::Enemy)
	{
		return std::nullopt;
	}
	for (const domain::BattleUnit& Unit : B.Units())
	{
		if (Unit.Side != domain::Faction::Enemy || Unit.HasActed)
		{
			continue;
		}
		if (const auto Plan = BestAttack(B, Unit, !Unit.HasMoved))
		{
			if (Plan->Cell == Unit.Position)
			{
				return BattleCommand(AttackUnitCommand{Unit.Id, Plan->Target});
			}
			return BattleCommand(MoveUnitCommand{Unit.Id, Plan->Cell});
		}
		if (!Unit.HasMoved)
		{
			const domain::GridCoord Cell = ApproachCell(B, Unit);
			if (!(Cell == Unit.Position))
			{
				return BattleCommand(MoveUnitCommand{Unit.Id, Cell});
			}
		}
		return BattleCommand(WaitUnitCommand{Unit.Id});
	}
	return BattleCommand(EndPhaseCommand{});
}

// Plays the whole enemy phase through the session and returns the Events in order. It stops early if a
// Command is rejected or the battle ends (then there is no EndPhase). Returns nothing outside the enemy phase.
inline std::vector<BattleEvent> RunEnemyPhase(BattleSession& Session)
{
	std::vector<BattleEvent> Events;
	const std::size_t Limit = Session.GetBattle().Units().size() * 3 + 1; // two Commands per unit + EndPhase
	for (std::size_t I = 0; I < Limit; ++I)
	{
		const std::optional<BattleCommand> Command = DecideNext(Session.GetBattle());
		if (!Command.has_value())
		{
			break;
		}
		const CommandResult Result = Session.Execute(*Command);
		if (!Result.IsOk())
		{
			break;
		}
		Events.push_back(Result.Value());
		if (std::holds_alternative<EndPhaseCommand>(*Command))
		{
			break;
		}
	}
	return Events;
}

} // namespace commander::application
