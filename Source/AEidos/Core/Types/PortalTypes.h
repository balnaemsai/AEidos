#pragma once

#include "CoreMinimal.h"
#include "PortalTypes.generated.h"

UENUM(BlueprintType)
enum class EPortalStatus : uint8
{
	Spawning,
	Idle,
	DungeonOpen,
	Cleared,
	RaidTriggered,
	Expired
};

USTRUCT(BlueprintType)
struct FPortalState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PortalId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Tier = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SpawnTime = 0.f;

	// 레이드 발생까지 남은 시간
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RaidTimer = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 DungeonSeed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPortalStatus Status = EPortalStatus::Idle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bDungeonEntered = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCleared = false;
};

USTRUCT(BlueprintType)
struct FEidosPortalSnapshot
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FPortalState> ActivePortals;

	UPROPERTY()
	int32 NextPortalId = 1;

	UPROPERTY()
	float TimeSinceLastSpawn = 0.f;
};