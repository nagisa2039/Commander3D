#include "Presentation/CommanderBattleBoard.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Presentation/CommanderBattleController.h"
#include "UObject/ConstructorHelpers.h"

ACommanderBattleBoard::ACommanderBattleBoard()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	// Engine primitive shapes: no project asset is needed.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	CubeMesh = Cube.Object;
	CylinderMesh = Cylinder.Object;
	BaseMaterial = Material.Object;
}

bool ACommanderBattleBoard::StartBattle()
{
	UGameInstance* GameInstance = GetGameInstance();
	UCommanderCampaignSubsystem* Campaign = GameInstance != nullptr ? GameInstance->GetSubsystem<UCommanderCampaignSubsystem>() : nullptr;
	if (Campaign == nullptr)
	{
		return false;
	}
	if (Controller == nullptr)
	{
		Controller = NewObject<UCommanderBattleController>(this);
		Controller->OnChanged.AddDynamic(this, &ACommanderBattleBoard::Redraw);
	}
	if (!Controller->BeginBattle(Campaign))
	{
		return false;
	}
	ClearPieces(TilePieces); // a new battle may have another map
	return true;
}

FVector ACommanderBattleBoard::CellToWorld(int32 X, int32 Y) const
{
	return GetActorTransform().TransformPosition(FVector((X + 0.5f) * CellSize, (Y + 0.5f) * CellSize, 0.f));
}

FIntPoint ACommanderBattleBoard::WorldToCell(const FVector& WorldLocation) const
{
	const FVector Local = GetActorTransform().InverseTransformPosition(WorldLocation);
	const int32 X = FMath::FloorToInt(Local.X / CellSize);
	const int32 Y = FMath::FloorToInt(Local.Y / CellSize);
	const FIntPoint Size = Controller != nullptr ? Controller->GetMapSize() : FIntPoint::ZeroValue;
	return (X >= 0 && Y >= 0 && X < Size.X && Y < Size.Y) ? FIntPoint(X, Y) : FIntPoint(-1, -1);
}

void ACommanderBattleBoard::HighlightCells(const TArray<FIntPoint>& Cells)
{
	Highlighted = Cells;
	Redraw();
}

UStaticMeshComponent* ACommanderBattleBoard::MakePiece(UStaticMesh* Mesh, const FVector& Location, const FVector& Scale, const FLinearColor& Color)
{
	UStaticMeshComponent* Piece = NewObject<UStaticMeshComponent>(this);
	Piece->SetStaticMesh(Mesh);
	Piece->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Piece->SetupAttachment(GetRootComponent());
	Piece->RegisterComponent();
	Piece->SetWorldLocation(Location);
	Piece->SetWorldScale3D(Scale);
	if (BaseMaterial != nullptr)
	{
		UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BaseMaterial, Piece);
		Material->SetVectorParameterValue(TEXT("Color"), Color);
		Piece->SetMaterial(0, Material);
	}
	return Piece;
}

void ACommanderBattleBoard::ClearPieces(TArray<TObjectPtr<UStaticMeshComponent>>& Pieces)
{
	for (UStaticMeshComponent* Piece : Pieces)
	{
		if (Piece != nullptr)
		{
			Piece->DestroyComponent();
		}
	}
	Pieces.Reset();
}

void ACommanderBattleBoard::Redraw()
{
	if (Controller == nullptr || CubeMesh == nullptr || CylinderMesh == nullptr)
	{
		return;
	}
	const float Unit = CellSize / 100.f; // the engine shapes are 100 uu wide

	// Tiles: once per battle. Dark = costly terrain, light = plain, hidden = impassable.
	if (TilePieces.IsEmpty())
	{
		const FIntPoint Size = Controller->GetMapSize();
		for (int32 Y = 0; Y < Size.Y; ++Y)
		{
			for (int32 X = 0; X < Size.X; ++X)
			{
				const int32 Cost = Controller->GetCellMoveCost(X, Y);
				const FLinearColor Color = Cost < 0 ? FLinearColor(0.05f, 0.05f, 0.05f) : (Cost > 1 ? FLinearColor(0.25f, 0.35f, 0.2f) : FLinearColor(0.55f, 0.6f, 0.5f));
				const float Height = Cost < 0 ? 0.6f : 0.1f; // walls stand up
				TilePieces.Add(MakePiece(CubeMesh, CellToWorld(X, Y) - FVector(0, 0, 5.f * Unit), FVector(0.96f * Unit, 0.96f * Unit, Height * Unit), Color));
			}
		}
	}

	ClearPieces(DynamicPieces);
	for (const FIntPoint& Cell : Highlighted)
	{
		DynamicPieces.Add(MakePiece(CubeMesh, CellToWorld(Cell.X, Cell.Y) + FVector(0, 0, 1.f), FVector(0.9f * Unit, 0.9f * Unit, 0.02f * Unit), FLinearColor(0.2f, 0.5f, 1.f)));
	}
	for (const FIntPoint& Cell : Controller->GetShopCells())
	{
		DynamicPieces.Add(MakePiece(CubeMesh, CellToWorld(Cell.X, Cell.Y) + FVector(0, 0, 10.f * Unit), FVector(0.4f * Unit, 0.4f * Unit, 0.2f * Unit), FLinearColor(0.9f, 0.8f, 0.2f)));
	}
	for (const FCommanderBattleChestView& Chest : Controller->GetChests())
	{
		DynamicPieces.Add(MakePiece(CubeMesh, CellToWorld(Chest.X, Chest.Y) + FVector(0, 0, 15.f * Unit), FVector(0.35f * Unit, 0.35f * Unit, 0.3f * Unit),
			Chest.bGold ? FLinearColor(1.f, 0.85f, 0.f) : FLinearColor(0.6f, 0.35f, 0.1f)));
	}
	for (const FCommanderBattleUnitView& View : Controller->GetUnits())
	{
		// Player: blue, enemy: red, boss: darker and taller; units that have acted are dimmed. Height shows Hp.
		const bool bDone = View.bHasActed;
		FLinearColor Color = View.bPlayer ? FLinearColor(0.1f, 0.3f, 0.9f) : (View.bBoss ? FLinearColor(0.5f, 0.f, 0.05f) : FLinearColor(0.85f, 0.15f, 0.1f));
		if (bDone)
		{
			Color *= 0.45f;
		}
		const float HpRatio = View.MaxHp > 0 ? FMath::Clamp(static_cast<float>(View.Hp) / View.MaxHp, 0.1f, 1.f) : 1.f;
		const float Height = (View.bBoss ? 1.4f : 1.f) * (0.5f + 0.5f * HpRatio);
		DynamicPieces.Add(MakePiece(CylinderMesh, CellToWorld(View.X, View.Y) + FVector(0, 0, 50.f * Height * Unit), FVector(0.6f * Unit, 0.6f * Unit, Height * Unit), Color));
	}
}
