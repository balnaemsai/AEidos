#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "World/Interaction/WorldBlockActor.h"
#include "DungeonSettlementPreset.generated.h"

class AWorldItemBlockActor;

/** Snapshot of one authored world block. Blocks cover terrain, props and harvestable objects alike. */
USTRUCT(BlueprintType)
struct FDungeonWorldBlockPreset
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon")
	TSoftClassPtr<AWorldBlockActor> ActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon")
	FTransform LocalTransform = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon")
	FName BlockId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon", meta=(ClampMin="1"))
	int32 RemainingIntegrity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon")
	TArray<FWorldBlockInteractionDefinition> Interactions;
};

USTRUCT(BlueprintType)
struct FDungeonSettlementBuildingPreset
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon")
	FName BuildingId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon")
	FTransform LocalTransform = FTransform::Identity;
};

USTRUCT(BlueprintType)
struct FDungeonEnemySpawnPreset
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon")
	FTransform LocalTransform = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon")
	int32 SpawnCount = 1;
};

/** A harvestable or portable world object authored as part of a dungeon settlement. */
USTRUCT(BlueprintType)
struct FDungeonWorldItemPreset
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon")
	TSoftClassPtr<AWorldItemBlockActor> ActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon")
	FTransform LocalTransform = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon")
	FName ItemId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon", meta=(ClampMin="1"))
	int32 RemainingQuantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon", meta=(ClampMin="1"))
	int32 QuantityPerHarvest = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon")
	bool bCanPickUp = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon")
	bool bCanHarvest = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon")
	FName RequiredHarvestToolTag = NAME_None;
};

UCLASS(BlueprintType)
class AEIDOS_API UDungeonSettlementPreset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon")
	FName PresetId = TEXT("DungeonPreset");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon")
	FName ThemeId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon")
	int32 SuggestedTier = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon")
	float ChunkSizeCm = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon")
	TArray<FIntPoint> OwnedChunks;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon")
	TArray<FDungeonSettlementBuildingPreset> Buildings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon")
	FTransform EntryTransform = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon")
	FTransform CoreTransform = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon")
	TArray<FDungeonEnemySpawnPreset> EnemySpawns;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon")
	TArray<FDungeonWorldBlockPreset> WorldBlocks;

	/** Legacy snapshots from before all world objects were unified as blocks. Kept so old authored presets still load. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon", meta=(DeprecatedProperty, DeprecationMessage="Use WorldBlocks."))
	TArray<FDungeonWorldItemPreset> WorldItems;
};
