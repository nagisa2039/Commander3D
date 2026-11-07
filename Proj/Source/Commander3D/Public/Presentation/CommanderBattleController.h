#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include <memory>
#include <vector>

#include "Battle/BattleSession.h"
#include "Battle/StdRandomSource.h"
#include "Presentation/CommanderCampaignSubsystem.h"

#include "CommanderBattleController.generated.h"

UENUM(BlueprintType)
enum class ECommanderBattleResult : uint8
{
	Ongoing,
	Won,
	Lost,
};

USTRUCT(BlueprintType)
struct COMMANDER3D_API FCommanderBattleUnitView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 Id = 0;
	UPROPERTY(BlueprintReadOnly) bool bPlayer = true;
	UPROPERTY(BlueprintReadOnly) bool bBoss = false;
	UPROPERTY(BlueprintReadOnly) int32 X = 0;
	UPROPERTY(BlueprintReadOnly) int32 Y = 0;
	UPROPERTY(BlueprintReadOnly) int32 Hp = 1;
	UPROPERTY(BlueprintReadOnly) int32 MaxHp = 1;
	UPROPERTY(BlueprintReadOnly) bool bHasMoved = false;
	UPROPERTY(BlueprintReadOnly) bool bHasActed = false;
};

USTRUCT(BlueprintType)
struct COMMANDER3D_API FCommanderBattleChestView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 X = 0;
	UPROPERTY(BlueprintReadOnly) int32 Y = 0;
	UPROPERTY(BlueprintReadOnly) bool bGold = false; // false: holds an item
};

// A line of text describing what just happened (for a log; the Presentation may also replay it as animation).
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCommanderBattleLogSignature, const FString&, Text);
// The battle state changed: refresh what is drawn.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCommanderBattleChangedSignature);
// The battle ended and was reported to the campaign.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCommanderBattleFinishedSignature, ECommanderBattleResult, Result, ECommanderCampaignStep, Step);

// Presentation: drives one battle for the screens. The player's input arrives as the commands below; each is
// forwarded to commander::application::BattleSession (rules stay in the Domain), the enemy phase is played with the
// enemy AI, and the result is translated into the view structs. When the battle ends it is reported to the
// campaign subsystem (Campaign::Complete). It needs no World, so it can be tested without one.
UCLASS(BlueprintType)
class COMMANDER3D_API UCommanderBattleController : public UObject
{
	GENERATED_BODY()

public:
	// Starts the current chapter's battle from the campaign's preparation. False if nobody is deployed, the setup is
	// invalid, or a battle is already running.
	UFUNCTION(BlueprintCallable, Category = "Commander|Battle")
	bool BeginBattle(UCommanderCampaignSubsystem* Campaign);

	// ---- queries ----
	UFUNCTION(BlueprintPure, Category = "Commander|Battle")
	bool IsBattleRunning() const { return Session != nullptr && !bFinished; }

	UFUNCTION(BlueprintPure, Category = "Commander|Battle")
	ECommanderBattleResult GetResult() const;

	UFUNCTION(BlueprintPure, Category = "Commander|Battle")
	ECommanderCampaignStep GetCampaignStep() const { return LastStep; }

	UFUNCTION(BlueprintPure, Category = "Commander|Battle")
	FIntPoint GetMapSize() const;

	// Move cost of a cell, or -1 if it is outside the map or impassable.
	UFUNCTION(BlueprintPure, Category = "Commander|Battle")
	int32 GetCellMoveCost(int32 X, int32 Y) const;

	UFUNCTION(BlueprintPure, Category = "Commander|Battle")
	int32 GetTurn() const;

	UFUNCTION(BlueprintPure, Category = "Commander|Battle")
	bool IsPlayerPhase() const;

	UFUNCTION(BlueprintPure, Category = "Commander|Battle")
	int32 GetGold() const;

	UFUNCTION(BlueprintPure, Category = "Commander|Battle")
	TArray<FCommanderBattleUnitView> GetUnits() const;

	UFUNCTION(BlueprintPure, Category = "Commander|Battle")
	TArray<FCommanderBattleChestView> GetChests() const;

	UFUNCTION(BlueprintPure, Category = "Commander|Battle")
	TArray<FIntPoint> GetShopCells() const;

	// Cells the unit can end its move on (empty if it cannot move now).
	UFUNCTION(BlueprintPure, Category = "Commander|Battle")
	TArray<FIntPoint> GetMovableCells(int32 UnitId) const;

	// Enemy units the unit could attack from where it stands.
	UFUNCTION(BlueprintPure, Category = "Commander|Battle")
	TArray<int32> GetAttackableTargets(int32 UnitId) const;

	// ---- commands (false = refused; nothing changed) ----
	UFUNCTION(BlueprintCallable, Category = "Commander|Battle")
	bool MoveUnit(int32 UnitId, int32 X, int32 Y);

	UFUNCTION(BlueprintCallable, Category = "Commander|Battle")
	bool WaitUnit(int32 UnitId);

	UFUNCTION(BlueprintCallable, Category = "Commander|Battle")
	bool AttackUnit(int32 AttackerId, int32 TargetId);

	UFUNCTION(BlueprintCallable, Category = "Commander|Battle")
	bool OpenChest(int32 UnitId);

	// Ends the player's phase and plays the whole enemy phase.
	UFUNCTION(BlueprintCallable, Category = "Commander|Battle")
	bool EndPlayerPhase();

	UPROPERTY(BlueprintAssignable, Category = "Commander|Battle")
	FCommanderBattleLogSignature OnLog;

	UPROPERTY(BlueprintAssignable, Category = "Commander|Battle")
	FCommanderBattleChangedSignature OnChanged;

	UPROPERTY(BlueprintAssignable, Category = "Commander|Battle")
	FCommanderBattleFinishedSignature OnFinished;

private:
	bool Run(const commander::application::BattleCommand& Command);
	void Announce(const commander::application::BattleEvent& Event);
	void ReportIfFinished();

	UPROPERTY()
	TObjectPtr<UCommanderCampaignSubsystem> CampaignSubsystem;

	bool bFinished = false;
	ECommanderCampaignStep LastStep = ECommanderCampaignStep::BattleOngoing;
	std::vector<commander::domain::UnitId> Deployed;
	std::unique_ptr<commander::application::StdRandomSource> Rng; // declared before Session: the session refers to it
	std::unique_ptr<commander::application::BattleSession> Session;
};
