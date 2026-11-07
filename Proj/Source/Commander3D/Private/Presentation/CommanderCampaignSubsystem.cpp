#include "Presentation/CommanderCampaignSubsystem.h"

#include "Infrastructure/CampaignLoader.h"
#include "Infrastructure/SaveStorage.h"
#include "Misc/Paths.h"

namespace
{
using namespace commander;

FString DataDir()
{
	return FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data"));
}

FString SaveDir()
{
	return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SaveGames"));
}

ECommanderItemKind ToView(domain::ItemKind Kind)
{
	switch (Kind)
	{
	case domain::ItemKind::Potion: return ECommanderItemKind::Potion;
	case domain::ItemKind::RepairKit: return ECommanderItemKind::RepairKit;
	case domain::ItemKind::Staff: return ECommanderItemKind::Staff;
	case domain::ItemKind::PromotionSeal: return ECommanderItemKind::PromotionSeal;
	case domain::ItemKind::Weapon: break;
	}
	return ECommanderItemKind::Weapon;
}

void FillWeapon(FCommanderItemView& View, const domain::WeaponSpec& Weapon)
{
	View.Might = Weapon.Might;
	View.Hit = Weapon.Hit;
	View.MinRange = Weapon.MinRange;
	View.MaxRange = Weapon.MaxRange;
	View.Durability = Weapon.Durability;
}

FCommanderItemView ToView(const domain::ItemSlot& Item)
{
	FCommanderItemView View;
	View.Kind = ToView(Item.Kind);
	View.Uses = Item.Uses;
	View.Amount = Item.Amount;
	View.Range = Item.Range;
	View.Price = Item.Price;
	if (Item.Kind == domain::ItemKind::Weapon)
	{
		FillWeapon(View, Item.Weapon);
	}
	return View;
}

// Slot indices come from Blueprints: a negative value must not wrap around to a huge size_t.
bool ToIndex(int32 Value, size_t& Out)
{
	if (Value < 0)
	{
		return false;
	}
	Out = static_cast<size_t>(Value);
	return true;
}

bool ToUnitId(int32 Value, domain::UnitId& Out)
{
	if (Value < 0)
	{
		return false;
	}
	Out = domain::UnitId{static_cast<uint32>(Value)};
	return true;
}
} // namespace

bool UCommanderCampaignSubsystem::StartCampaign(const FString& InCampaignName, bool bClassic)
{
	const auto Loaded = FCampaignLoader::Load(DataDir(), InCampaignName);
	if (!Loaded.IsOk())
	{
		return false;
	}
	CampaignName = InCampaignName;
	Run = std::make_unique<application::Campaign>(Loaded.Value().InitialRoster, bClassic ? domain::Difficulty::Classic : domain::Difficulty::Casual,
		Loaded.Value().Chapters);
	return true;
}

bool UCommanderCampaignSubsystem::SaveGame(const FString& Slot)
{
	return Run != nullptr && FSaveStorage::Save(DataDir(), SaveDir(), Slot, CampaignName, *Run) == ESaveError::None;
}

bool UCommanderCampaignSubsystem::LoadGame(const FString& Slot)
{
	FString SavedName;
	const auto Loaded = FSaveStorage::Load(DataDir(), SaveDir(), Slot, &SavedName);
	if (!Loaded.IsOk())
	{
		return false;
	}
	CampaignName = SavedName; // so that a later SaveGame writes the same campaign
	Run = std::make_unique<application::Campaign>(Loaded.Value());
	return true;
}

FString UCommanderCampaignSubsystem::GetChapterName() const
{
	return (Run != nullptr && !Run->IsFinished()) ? FString(UTF8_TO_TCHAR(Run->CurrentChapter().Name.c_str())) : FString();
}

TArray<FCommanderUnitView> UCommanderCampaignSubsystem::GetRoster() const
{
	TArray<FCommanderUnitView> Views;
	if (Run == nullptr)
	{
		return Views;
	}
	const auto& Deployed = Run->Preparation().Deployed();
	for (const domain::BattleUnit& Unit : Run->Roster().Members())
	{
		FCommanderUnitView View;
		View.Id = static_cast<int32>(Unit.Id.Value);
		View.Level = Unit.Level;
		View.Experience = Unit.Experience;
		View.MaxHp = Unit.MaxHp;
		View.Strength = Unit.Strength;
		View.Skill = Unit.Skill;
		View.Speed = Unit.Speed;
		View.Defense = Unit.Defense;
		View.Movement = Unit.Movement;
		View.bDeployed = std::find(Deployed.begin(), Deployed.end(), Unit.Id) != Deployed.end();
		View.bHasWeapon = Unit.Weapon.MaxRange > 0;
		if (View.bHasWeapon)
		{
			View.Weapon.Kind = ECommanderItemKind::Weapon;
			View.Weapon.Uses = Unit.WeaponUses;
			FillWeapon(View.Weapon, Unit.Weapon);
		}
		for (const domain::ItemSlot& Item : Unit.Items)
		{
			View.Items.Add(ToView(Item));
		}
		Views.Add(View);
	}
	return Views;
}

TArray<FCommanderItemView> UCommanderCampaignSubsystem::GetStock() const
{
	TArray<FCommanderItemView> Views;
	if (Run != nullptr && !Run->IsFinished())
	{
		for (const domain::ItemSlot& Item : Run->Preparation().Stock())
		{
			Views.Add(ToView(Item));
		}
	}
	return Views;
}

int32 UCommanderCampaignSubsystem::GetMaxDeployed() const
{
	return (Run != nullptr && !Run->IsFinished()) ? static_cast<int32>(Run->Preparation().MaxDeployed()) : 0;
}

int32 UCommanderCampaignSubsystem::GetDeployedCount() const
{
	return (Run != nullptr && !Run->IsFinished()) ? static_cast<int32>(Run->Preparation().Deployed().size()) : 0;
}

bool UCommanderCampaignSubsystem::Deploy(int32 UnitId)
{
	domain::UnitId Id;
	return Run != nullptr && !Run->IsFinished() && ToUnitId(UnitId, Id) && Run->Preparation().Deploy(Id);
}

bool UCommanderCampaignSubsystem::Withdraw(int32 UnitId)
{
	domain::UnitId Id;
	return Run != nullptr && !Run->IsFinished() && ToUnitId(UnitId, Id) && Run->Preparation().Withdraw(Id);
}

bool UCommanderCampaignSubsystem::BuyItem(int32 UnitId, int32 Entry)
{
	domain::UnitId Id;
	size_t Index = 0;
	return Run != nullptr && !Run->IsFinished() && ToUnitId(UnitId, Id) && ToIndex(Entry, Index) && Run->Preparation().BuyItem(Id, Index);
}

bool UCommanderCampaignSubsystem::SellItem(int32 UnitId, int32 Slot)
{
	domain::UnitId Id;
	size_t Index = 0;
	return Run != nullptr && !Run->IsFinished() && ToUnitId(UnitId, Id) && ToIndex(Slot, Index) && Run->Preparation().SellItem(Id, Index);
}

bool UCommanderCampaignSubsystem::EquipWeapon(int32 UnitId, int32 Slot)
{
	domain::UnitId Id;
	size_t Index = 0;
	return Run != nullptr && !Run->IsFinished() && ToUnitId(UnitId, Id) && ToIndex(Slot, Index) && Run->Preparation().EquipWeapon(Id, Index);
}

bool UCommanderCampaignSubsystem::TransferItem(int32 FromUnitId, int32 ToUnitId_, int32 Slot)
{
	domain::UnitId From;
	domain::UnitId To;
	size_t Index = 0;
	return Run != nullptr && !Run->IsFinished() && ToUnitId(FromUnitId, From) && ToUnitId(ToUnitId_, To) && ToIndex(Slot, Index)
		&& Run->Preparation().TransferItem(From, To, Index);
}

bool UCommanderCampaignSubsystem::DiscardItem(int32 UnitId, int32 Slot)
{
	domain::UnitId Id;
	size_t Index = 0;
	return Run != nullptr && !Run->IsFinished() && ToUnitId(UnitId, Id) && ToIndex(Slot, Index) && Run->Preparation().DiscardItem(Id, Index);
}

std::optional<application::PreparedBattle> UCommanderCampaignSubsystem::StartBattle() const
{
	if (Run == nullptr || Run->IsFinished())
	{
		return std::nullopt;
	}
	auto Started = Run->Preparation().Start();
	if (!Started.IsOk())
	{
		return std::nullopt;
	}
	return Started.Value();
}

ECommanderCampaignStep UCommanderCampaignSubsystem::CompleteBattle(const domain::Battle& Finished, const std::vector<domain::UnitId>& Sortied)
{
	if (Run == nullptr || Run->IsFinished())
	{
		return ECommanderCampaignStep::BattleOngoing;
	}
	switch (Run->Complete(Finished, Sortied))
	{
	case application::CampaignStep::Retry: return ECommanderCampaignStep::Retry;
	case application::CampaignStep::NextChapter: return ECommanderCampaignStep::NextChapter;
	case application::CampaignStep::CampaignCleared: return ECommanderCampaignStep::CampaignCleared;
	case application::CampaignStep::BattleOngoing: break;
	}
	return ECommanderCampaignStep::BattleOngoing;
}
