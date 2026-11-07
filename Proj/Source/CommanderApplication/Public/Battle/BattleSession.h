#pragma once

#include <optional>
#include <utility>
#include <variant>

#include "Battle/Battle.h"
#include "Common/Result.h"

namespace commander::application
{

// Commands: what a player (or later the enemy AI) wants to do.
struct MoveUnitCommand
{
	domain::UnitId Unit;
	domain::GridCoord Destination;
};

struct WaitUnitCommand
{
	domain::UnitId Unit;
};

struct AttackUnitCommand
{
	domain::UnitId Attacker;
	domain::UnitId Target;
};

struct EndPhaseCommand
{
};

using BattleCommand = std::variant<MoveUnitCommand, WaitUnitCommand, AttackUnitCommand, EndPhaseCommand>;

// Events: what happened, for Presentation to replay.
using BattleEvent = std::variant<domain::UnitMoved, domain::UnitWaited, domain::AttackResolved, domain::PhaseChanged>;

enum class CommandError
{
	BattleEnded, // the battle already has an outcome
	Rejected,    // the Domain refused it (rule violation); state is unchanged. Reasons are not distinguished yet.
};

using CommandResult = Result<BattleEvent, CommandError>;

// Runs Commands against one Battle. The only entry point for state changes from outside the Domain.
// Rng must outlive the session.
class BattleSession
{
public:
	BattleSession(domain::Battle InBattle, domain::IRandomSource& InRng) : BattleState(std::move(InBattle)), Rng(InRng) {}

	const domain::Battle& GetBattle() const { return BattleState; }

	CommandResult Execute(const BattleCommand& Command)
	{
		if (BattleState.Outcome() != domain::BattleOutcome::Ongoing)
		{
			return CommandResult::Err(CommandError::BattleEnded);
		}
		return std::visit([this](const auto& C) { return Run(C); }, Command);
	}

private:
	template <typename Event>
	static CommandResult FromOptional(std::optional<Event> Result)
	{
		if (!Result.has_value())
		{
			return CommandResult::Err(CommandError::Rejected);
		}
		return CommandResult::Ok(BattleEvent(std::move(*Result)));
	}

	CommandResult Run(const MoveUnitCommand& C) { return FromOptional(BattleState.MoveUnit(C.Unit, C.Destination)); }
	CommandResult Run(const WaitUnitCommand& C) { return FromOptional(BattleState.WaitUnit(C.Unit)); }
	CommandResult Run(const AttackUnitCommand& C) { return FromOptional(BattleState.AttackUnit(C.Attacker, C.Target, Rng)); }
	CommandResult Run(const EndPhaseCommand&) { return CommandResult::Ok(BattleEvent(BattleState.EndPhase())); }

	domain::Battle BattleState;
	domain::IRandomSource& Rng;
};

} // namespace commander::application
