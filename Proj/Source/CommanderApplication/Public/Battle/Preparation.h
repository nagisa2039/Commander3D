#pragma once

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

#include "Battle/BattleSetup.h"
#include "Common/Result.h"
#include "Roster/Roster.h"

namespace commander::application
{

enum class PreparationError
{
	NoUnitDeployed,
	InvalidSetup, // the battle setup itself is invalid (see StartBattle)
};

struct PreparedBattle
{
	domain::Battle Battle;
	std::vector<domain::UnitId> Deployed; // pass back to Preparation::Finish
};

// The screen before a battle: the player chooses who sorties (up to one per start cell), reorganizes the roster's
// items (equip, hand over, discard) and then starts. Base holds the map, enemies, conditions and shops; its Gold
// and player units are replaced from the roster. The Presentation shows this state and forwards the player's input.
class Preparation
{
public:
	Preparation(domain::Roster InRoster, BattleSetup InBase, std::vector<domain::GridCoord> InStartCells)
		: RosterState(std::move(InRoster)), Base(std::move(InBase)), StartCells(std::move(InStartCells))
	{
	}

	const domain::Roster& Roster() const { return RosterState; }
	const std::vector<domain::UnitId>& Deployed() const { return DeployedIds; }
	std::size_t MaxDeployed() const { return StartCells.size(); }

	// Sorties a roster member. False if unknown, already deployed or every start cell is taken.
	bool Deploy(domain::UnitId Id)
	{
		if (RosterState.Find(Id) == nullptr || IsDeployed(Id) || DeployedIds.size() >= StartCells.size())
		{
			return false;
		}
		DeployedIds.push_back(Id);
		return true;
	}

	bool Withdraw(domain::UnitId Id)
	{
		const auto It = std::find(DeployedIds.begin(), DeployedIds.end(), Id);
		if (It == DeployedIds.end())
		{
			return false;
		}
		DeployedIds.erase(It);
		return true;
	}

	// The items the chapter's preparation shop sells (never runs out; each Price is the buying price). May be empty.
	const std::vector<domain::ItemSlot>& Stock() const { return Base.PreparationStock; }

	// Shopping with the roster's gold; items are sold for half their price. False if the member or entry/slot is
	// missing, the pack is full, gold is short, or the item has no price.
	bool BuyItem(domain::UnitId Id, std::size_t Entry) { return RosterState.BuyItem(Id, Base.PreparationStock, Entry); }
	bool SellItem(domain::UnitId Id, std::size_t Slot) { return RosterState.SellItem(Id, Slot); }

	bool EquipWeapon(domain::UnitId Id, std::size_t Slot) { return RosterState.EquipWeapon(Id, Slot); }
	bool TransferItem(domain::UnitId From, domain::UnitId To, std::size_t Slot) { return RosterState.TransferItem(From, To, Slot); }
	bool DiscardItem(domain::UnitId Id, std::size_t Slot) { return RosterState.DiscardItem(Id, Slot); }

	// Builds the battle: deployed members stand on the start cells in the order they were deployed.
	Result<PreparedBattle, PreparationError> Start() const
	{
		using Outcome = Result<PreparedBattle, PreparationError>;
		if (DeployedIds.empty())
		{
			return Outcome::Err(PreparationError::NoUnitDeployed);
		}

		BattleSetup Setup = Base;
		Setup.Gold = RosterState.Gold();
		for (std::size_t I = 0; I < DeployedIds.size(); ++I)
		{
			auto Unit = RosterState.Deploy(DeployedIds[I], StartCells[I]);
			if (!Unit)
			{
				return Outcome::Err(PreparationError::InvalidSetup);
			}
			Setup.Units.push_back(*Unit);
		}

		auto Started = StartBattle(Setup);
		if (!Started.IsOk())
		{
			return Outcome::Err(PreparationError::InvalidSetup);
		}
		return Outcome::Ok(PreparedBattle{Started.Value(), DeployedIds});
	}

	// Carries a finished battle back into the roster (see Roster::ApplyBattleResult) and clears the deployment.
	void Finish(const domain::Battle& Finished, const std::vector<domain::UnitId>& Sortied, domain::Difficulty Mode)
	{
		RosterState.ApplyBattleResult(Finished, Sortied, Mode);
		DeployedIds.clear();
	}

private:
	bool IsDeployed(domain::UnitId Id) const
	{
		return std::find(DeployedIds.begin(), DeployedIds.end(), Id) != DeployedIds.end();
	}

	domain::Roster RosterState;
	BattleSetup Base;
	std::vector<domain::GridCoord> StartCells;
	std::vector<domain::UnitId> DeployedIds;
};

} // namespace commander::application
