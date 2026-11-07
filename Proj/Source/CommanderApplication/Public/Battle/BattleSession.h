#pragma once

#include <cstddef>
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

struct EquipWeaponCommand
{
	domain::UnitId Unit;
	std::size_t Slot = 0; // inventory slot holding the weapon to equip
};

struct UseItemCommand
{
	domain::UnitId Unit;
	std::size_t Slot = 0; // inventory slot holding a potion or repair kit
};

struct GiveItemCommand
{
	domain::UnitId Giver;
	domain::UnitId Receiver;
	std::size_t Slot = 0; // the giver's inventory slot
};

struct DropItemCommand
{
	domain::UnitId Unit;
	std::size_t Slot = 0;
};

struct UseStaffCommand
{
	domain::UnitId Healer;
	domain::UnitId Target;
	std::size_t Slot = 0; // the healer's slot holding the staff
};

struct BuyItemCommand
{
	domain::UnitId Unit; // must stand on a shop
	std::size_t Entry = 0; // index into the shop's stock
};

struct SellItemCommand
{
	domain::UnitId Unit; // must stand on a shop
	std::size_t Slot = 0;
};

struct PromoteUnitCommand
{
	domain::UnitId Unit;
	std::size_t Option = 0;                  // which of the unit's promotion options to take
	std::optional<std::size_t> SealSlot;     // spend the promotion seal in this slot to promote below the normal level
};

struct EndPhaseCommand
{
};

using BattleCommand = std::variant<MoveUnitCommand, WaitUnitCommand, AttackUnitCommand, EquipWeaponCommand, UseItemCommand, GiveItemCommand,
	DropItemCommand, UseStaffCommand, BuyItemCommand, SellItemCommand, PromoteUnitCommand, EndPhaseCommand>;

// Events: what happened, for Presentation to replay.
using BattleEvent = std::variant<domain::UnitMoved, domain::UnitWaited, domain::AttackResolved, domain::WeaponEquipped, domain::ItemUsed,
	domain::ItemGiven, domain::ItemDropped, domain::StaffUsed, domain::ItemBought, domain::ItemSold, domain::UnitPromoted, domain::PhaseChanged>;

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
	CommandResult Run(const EquipWeaponCommand& C) { return FromOptional(BattleState.EquipWeapon(C.Unit, C.Slot)); }
	CommandResult Run(const UseItemCommand& C) { return FromOptional(BattleState.UseItem(C.Unit, C.Slot)); }
	CommandResult Run(const GiveItemCommand& C) { return FromOptional(BattleState.GiveItem(C.Giver, C.Receiver, C.Slot)); }
	CommandResult Run(const DropItemCommand& C) { return FromOptional(BattleState.DropItem(C.Unit, C.Slot)); }
	CommandResult Run(const UseStaffCommand& C) { return FromOptional(BattleState.UseStaff(C.Healer, C.Target, C.Slot, Rng)); }
	CommandResult Run(const BuyItemCommand& C) { return FromOptional(BattleState.BuyItem(C.Unit, C.Entry)); }
	CommandResult Run(const SellItemCommand& C) { return FromOptional(BattleState.SellItem(C.Unit, C.Slot)); }
	CommandResult Run(const PromoteUnitCommand& C) { return FromOptional(BattleState.PromoteUnit(C.Unit, C.Option, C.SealSlot)); }
	CommandResult Run(const EndPhaseCommand&) { return CommandResult::Ok(BattleEvent(BattleState.EndPhase())); }

	domain::Battle BattleState;
	domain::IRandomSource& Rng;
};

} // namespace commander::application
