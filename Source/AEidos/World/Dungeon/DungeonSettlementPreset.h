#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DungeonSettlementPreset.generated.h"

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
};
