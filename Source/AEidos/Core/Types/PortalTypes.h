#pragma once

#include "CoreMinimal.h"
#include "PortalTypes.generated.h"

UENUM(BlueprintType)
enum class EPortalStatus : uint8
{
	// Spawn request has been committed, but gameplay interaction is not yet open.
	Spawning,
	// Actively present in the world and counting down toward a raid trigger.
	Available,
	// A party has committed to the portal, so it should no longer behave like an ignored threat.
	Entered,
	// Portal objective was resolved successfully.
	Cleared,
	// Ignore timer elapsed and downstream penalty/raid should fire.
	RaidTriggered,
	// Terminal cleanup state after completion or raid resolution.
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
	EPortalStatus Status = EPortalStatus::Spawning;

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
