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

// Terrain-only path cost from the cell of each unit matching IsTarget to every cell (units do not block, so a
// crowd of allies cannot make a target look unreachable). One field per matching unit.
template <typename Predicate>
std::vector<domain::MovementRange> PathCostFields(const domain::Battle& B, Predicate IsTarget)
{
	constexpr std::int32_t Unlimited = 1 << 20; // far above any real map's path cost
	std::vector<domain::MovementRange> Fields;
	for (const domain::BattleUnit& Other : B.Units())
	{
		if (IsTarget(Other))
		{
			domain::BattleUnit Probe = Other;
			Probe.Movement = Unlimited;
			Fields.push_back(domain::CalculateMovementRange(B.GetMap(), {}, Probe));
		}
	}
	return Fields;
}

// The reachable cell with the shortest terrain path (movement-cost weighted) to any unit matching IsTarget;
// ties go to the cheaper move. The unit's own cell when nothing is closer. Targets that cannot be reached by
// any path (walled off) are ignored.
template <typename Predicate>
domain::GridCoord ApproachCell(const domain::Battle& B, const domain::BattleUnit& Unit, Predicate IsTarget)
{
	const std::vector<domain::MovementRange> Fields = PathCostFields(B, IsTarget);
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

// ---- healing ----

// A hurt ally is worth a staff charge once it is at or below this share of its maximum Hp.
inline constexpr std::int32_t HealBelowPercent = 75;

inline bool NeedsHealing(const domain::BattleUnit& Ally)
{
	return Ally.Hp * 100 <= Ally.MaxHp * HealBelowPercent;
}

inline bool HasStaff(const domain::BattleUnit& Unit)
{
	for (const domain::ItemSlot& Item : Unit.Items)
	{
		if (Item.Kind == domain::ItemKind::Staff)
		{
			return true;
		}
	}
	return false;
}

struct HealPlan
{
	domain::GridCoord Cell;
	domain::UnitId Target;
	std::size_t Slot = 0;
	std::int32_t Cost = 0;
	std::int32_t Hp = 0;
	std::int32_t MaxHp = 1;
};

// The most wounded (lowest Hp share) first, then the lowest Hp, then the cheaper move, then the lower target id.
inline bool IsBetterHeal(const HealPlan& A, const HealPlan& B)
{
	const std::int32_t ShareA = A.Hp * B.MaxHp;
	const std::int32_t ShareB = B.Hp * A.MaxHp;
	if (ShareA != ShareB)
	{
		return ShareA < ShareB;
	}
	if (A.Hp != B.Hp)
	{
		return A.Hp < B.Hp;
	}
	if (A.Cost != B.Cost)
	{
		return A.Cost < B.Cost;
	}
	return A.Target.Value < B.Target.Value;
}

// Best (cell, ally, staff) to heal with: an ally on the unit's side, other than itself, that NeedsHealing and
// is within the staff's range (1..Range) from a cell the unit can stand on.
inline std::optional<HealPlan> BestHeal(const domain::Battle& B, const domain::BattleUnit& Unit, bool CanMove)
{
	std::optional<HealPlan> Best;
	for (std::size_t Slot = 0; Slot < Unit.Items.size(); ++Slot)
	{
		const domain::ItemSlot& Staff = Unit.Items[Slot];
		if (Staff.Kind != domain::ItemKind::Staff)
		{
			continue;
		}
		for (const auto& [Cell, Cost] : CandidateCells(B, Unit, CanMove))
		{
			for (const domain::BattleUnit& Ally : B.Units())
			{
				const std::int32_t Distance = domain::ManhattanDistance(Cell, Ally.Position);
				if (Ally.Side != Unit.Side || Ally.Id == Unit.Id || !NeedsHealing(Ally) || Distance < 1 || Distance > Staff.Range)
				{
					continue;
				}
				HealPlan Plan;
				Plan.Cell = Cell;
				Plan.Target = Ally.Id;
				Plan.Slot = Slot;
				Plan.Cost = Cost;
				Plan.Hp = Ally.Hp;
				Plan.MaxHp = Ally.MaxHp;
				if (!Best.has_value() || IsBetterHeal(Plan, *Best))
				{
					Best = Plan;
				}
			}
		}
	}
	return Best;
}

} // namespace enemy_ai_detail

// The next Command the enemy side wants to issue, or nullopt when it is not the enemy phase or the battle is over.
// Enemy units act in list order:
//   - a unit with no usable weapon swaps in a usable spare;
//   - a healer (staff in the pack) heals the most wounded ally it can reach;
//   - otherwise attack the best target (moving into range if needed);
//   - otherwise approach the nearest player unit (an unarmed healer instead follows its hurt allies), else wait.
// Enemies never use potions or repair kits. When every enemy unit has acted the answer is EndPhaseCommand.
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
		// A broken weapon is swapped for the first usable spare, which is free and lets the unit fight on.
		if (!domain::HasUsableWeapon(Unit))
		{
			for (std::size_t Slot = 0; Slot < Unit.Items.size(); ++Slot)
			{
				const domain::ItemSlot& Item = Unit.Items[Slot];
				// CanUseWeapon also covers the rank requirement, which EquipWeapon enforces too.
				if (Item.Kind == domain::ItemKind::Weapon && domain::CanUseWeapon(Unit, Item.Weapon, Item.Uses))
				{
					return BattleCommand(EquipWeaponCommand{Unit.Id, Slot});
				}
			}
		}
		// A healer heals first: the most wounded ally it can reach (moving into staff range if needed).
		if (const auto Heal = BestHeal(B, Unit, !Unit.HasMoved))
		{
			if (Heal->Cell == Unit.Position)
			{
				return BattleCommand(UseStaffCommand{Unit.Id, Heal->Target, Heal->Slot});
			}
			return BattleCommand(MoveUnitCommand{Unit.Id, Heal->Cell});
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
			// An unarmed healer stays with its hurt allies instead of walking into the player's units; everyone
			// else closes in on the nearest player unit.
			const bool bSupportOnly = HasStaff(Unit) && !domain::HasUsableWeapon(Unit);
			const domain::Faction OwnSide = Unit.Side;
			const domain::UnitId Self = Unit.Id;
			const domain::GridCoord Cell = bSupportOnly
				? ApproachCell(B, Unit, [OwnSide, Self](const domain::BattleUnit& Other) {
					  return Other.Side == OwnSide && !(Other.Id == Self) && Other.Hp < Other.MaxHp;
				  })
				: ApproachCell(B, Unit, [](const domain::BattleUnit& Other) { return Other.Side == domain::Faction::Player; });
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
