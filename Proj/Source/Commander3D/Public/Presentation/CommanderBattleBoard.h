#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "CommanderBattleBoard.generated.h"

class UCommanderBattleController;
class UMaterialInterface;
class UStaticMesh;
class UCommanderCampaignSubsystem;
class UStaticMeshComponent;

// Presentation: draws a battle on the map with engine primitive shapes (a tile per cell, a cylinder per unit, small
// cubes for chests and shops) so that it is visible without any art asset. It owns a UCommanderBattleController,
// redraws whenever the controller reports a change. Place one in the level, call StartBattle, and drive the
// battle through GetController() (Blueprint input handling can use CellToWorld / WorldToCell / HighlightCells).
// Dressing it up with real meshes needs the editor and is not done here.
UCLASS()
class COMMANDER3D_API ACommanderBattleBoard : public AActor
{
	GENERATED_BODY()

public:
	ACommanderBattleBoard();

	// Starts the current chapter's battle from the campaign subsystem's preparation and draws it.
	UFUNCTION(BlueprintCallable, Category = "Commander|Battle")
	bool StartBattle();

	UFUNCTION(BlueprintPure, Category = "Commander|Battle")
	UCommanderBattleController* GetController() const { return Controller; }

	// World position of a cell's centre (the board's origin is the cell (0, 0) corner).
	UFUNCTION(BlueprintPure, Category = "Commander|Battle")
	FVector CellToWorld(int32 X, int32 Y) const;

	// Cell under a world position, or (-1, -1) if it is outside the map.
	UFUNCTION(BlueprintPure, Category = "Commander|Battle")
	FIntPoint WorldToCell(const FVector& WorldLocation) const;

	// Highlights cells (e.g. the movable cells of the selected unit). An empty array clears it.
	UFUNCTION(BlueprintCallable, Category = "Commander|Battle")
	void HighlightCells(const TArray<FIntPoint>& Cells);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Commander|Battle")
	float CellSize = 100.f;

private:
	UFUNCTION()
	void Redraw();

	UStaticMeshComponent* MakePiece(UStaticMesh* Mesh, const FVector& Location, const FVector& Scale, const FLinearColor& Color);
	void ClearPieces(TArray<TObjectPtr<UStaticMeshComponent>>& Pieces);

	UPROPERTY()
	TObjectPtr<UCommanderBattleController> Controller;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CylinderMesh;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> TilePieces;   // built once per battle

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> DynamicPieces; // units, chests, shops, highlights: rebuilt on every change

	TArray<FIntPoint> Highlighted;
};
