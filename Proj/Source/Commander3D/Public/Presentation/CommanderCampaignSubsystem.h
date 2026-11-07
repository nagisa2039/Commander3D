#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include <memory>
#include <optional>

#include "Battle/Campaign.h"

#include "CommanderCampaignSubsystem.generated.h"

UENUM(BlueprintType)
enum class ECommanderItemKind : uint8
{
	Weapon,
	Potion,
	RepairKit,
	Staff,
	PromotionSeal,
};

UENUM(BlueprintType)
enum class ECommanderCampaignStep : uint8
{
	BattleOngoing,
	Retry,
	NextChapter,
	CampaignCleared,
};

// What the screens show for one item / weapon. Only the fields of its Kind are meaningful.
USTRUCT(BlueprintType)
struct COMMANDER3D_API FCommanderItemView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) ECommanderItemKind Kind = ECommanderItemKind::Weapon;
	UPROPERTY(BlueprintReadOnly) int32 Uses = 0;    // Weapon: uses left (0 with Durability 0 = never breaks); others: charges left
	UPROPERTY(BlueprintReadOnly) int32 Amount = 0;  // Potion / Staff: Hp restored; PromotionSeal: level needed
	UPROPERTY(BlueprintReadOnly) int32 Range = 0;   // Staff: reach
	UPROPERTY(BlueprintReadOnly) int32 Price = 0;   // buying price (0 = not for sale); it sells for half
	UPROPERTY(BlueprintReadOnly) int32 Might = 0;   // Weapon
	UPROPERTY(BlueprintReadOnly) int32 Hit = 0;
	UPROPERTY(BlueprintReadOnly) int32 MinRange = 0;
	UPROPERTY(BlueprintReadOnly) int32 MaxRange = 0;
	UPROPERTY(BlueprintReadOnly) int32 Durability = 0;
};

// One roster member as the preparation screen shows it.
USTRUCT(BlueprintType)
struct COMMANDER3D_API FCommanderUnitView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 Id = 0;
	UPROPERTY(BlueprintReadOnly) int32 Level = 1;
	UPROPERTY(BlueprintReadOnly) int32 Experience = 0;
	UPROPERTY(BlueprintReadOnly) int32 MaxHp = 1;
	UPROPERTY(BlueprintReadOnly) int32 Strength = 0;
	UPROPERTY(BlueprintReadOnly) int32 Skill = 0;
	UPROPERTY(BlueprintReadOnly) int32 Speed = 0;
	UPROPERTY(BlueprintReadOnly) int32 Defense = 0;
	UPROPERTY(BlueprintReadOnly) int32 Movement = 0;
	UPROPERTY(BlueprintReadOnly) bool bDeployed = false;
	UPROPERTY(BlueprintReadOnly) bool bHasWeapon = false;
	UPROPERTY(BlueprintReadOnly) FCommanderItemView Weapon; // the equipped weapon (Kind = Weapon), valid when bHasWeapon
	UPROPERTY(BlueprintReadOnly) TArray<FCommanderItemView> Items;
};

// Presentation: the Blueprint-facing entry point for the campaign screens (preparation, shop, chapter progress, save
// and load). It holds no rules: every call is forwarded to commander::application::Campaign / Preparation and the
// result is translated into the view structs above. Widget Blueprints made in the editor call these functions.
UCLASS()
class COMMANDER3D_API UCommanderCampaignSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ---- run lifecycle ----
	// Starts a new run of <Content>/Data/Campaigns/<CampaignName>.json. False if it does not load.
	UFUNCTION(BlueprintCallable, Category = "Commander|Campaign")
	bool StartCampaign(const FString& CampaignName, bool bClassic);

	UFUNCTION(BlueprintCallable, Category = "Commander|Campaign")
	bool SaveGame(const FString& Slot);

	UFUNCTION(BlueprintCallable, Category = "Commander|Campaign")
	bool LoadGame(const FString& Slot);

	UFUNCTION(BlueprintPure, Category = "Commander|Campaign")
	bool HasCampaign() const { return Run != nullptr; }

	UFUNCTION(BlueprintPure, Category = "Commander|Campaign")
	bool IsCampaignFinished() const { return Run != nullptr && Run->IsFinished(); }

	// ---- progress ----
	UFUNCTION(BlueprintPure, Category = "Commander|Campaign")
	FString GetChapterName() const;

	UFUNCTION(BlueprintPure, Category = "Commander|Campaign")
	int32 GetChapterIndex() const { return Run != nullptr ? static_cast<int32>(Run->ChapterIndex()) : 0; }

	UFUNCTION(BlueprintPure, Category = "Commander|Campaign")
	int32 GetGold() const { return Run != nullptr ? Run->Roster().Gold() : 0; }

	// ---- preparation ----
	UFUNCTION(BlueprintPure, Category = "Commander|Preparation")
	TArray<FCommanderUnitView> GetRoster() const;

	// What the chapter's preparation shop sells.
	UFUNCTION(BlueprintPure, Category = "Commander|Preparation")
	TArray<FCommanderItemView> GetStock() const;

	UFUNCTION(BlueprintPure, Category = "Commander|Preparation")
	int32 GetMaxDeployed() const;

	UFUNCTION(BlueprintPure, Category = "Commander|Preparation")
	int32 GetDeployedCount() const;

	UFUNCTION(BlueprintCallable, Category = "Commander|Preparation")
	bool Deploy(int32 UnitId);

	UFUNCTION(BlueprintCallable, Category = "Commander|Preparation")
	bool Withdraw(int32 UnitId);

	UFUNCTION(BlueprintCallable, Category = "Commander|Preparation")
	bool BuyItem(int32 UnitId, int32 Entry);

	UFUNCTION(BlueprintCallable, Category = "Commander|Preparation")
	bool SellItem(int32 UnitId, int32 Slot);

	UFUNCTION(BlueprintCallable, Category = "Commander|Preparation")
	bool EquipWeapon(int32 UnitId, int32 Slot);

	UFUNCTION(BlueprintCallable, Category = "Commander|Preparation")
	bool TransferItem(int32 FromUnitId, int32 ToUnitId, int32 Slot);

	UFUNCTION(BlueprintCallable, Category = "Commander|Preparation")
	bool DiscardItem(int32 UnitId, int32 Slot);

	// ---- battle hand-over (C++ only: the battle is not a Blueprint type) ----
	// Builds the chapter's battle from the preparation. Nullopt if nobody is deployed or the setup is invalid.
	std::optional<commander::application::PreparedBattle> StartBattle() const;

	// Reports the finished battle (see Campaign::Complete).
	ECommanderCampaignStep CompleteBattle(const commander::domain::Battle& Finished, const std::vector<commander::domain::UnitId>& Sortied);

private:
	FString CampaignName;
	std::unique_ptr<commander::application::Campaign> Run;
};
