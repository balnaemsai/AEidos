// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/Types/DungeonTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "Simulation/SimSystem.h"
#include "WS_RaidDirector.generated.h"

class APageCharacter;
class ASettlementCoreActor;
class USimCommandBuffer;
struct FPortalState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSettlementDefeated);

USTRUCT()
struct FRaidRuntime
{
	GENERATED_BODY()

	int32 PortalId = INDEX_NONE;
	float DungeonDifficulty = 1.f;
	float SettlementValueAtSpawn = 0.f;
	TArray<FDungeonAttributeWeight> DungeonAttributes;
	int32 CurrentWave = 0;
	int32 TotalWaves = 0;
	FVector PortalLocation = FVector::ZeroVector;
	TArray<TWeakObjectPtr<APageCharacter>> Raiders;
	float RepathCooldownSeconds = 0.f;
	// All raiders are gone, but the warehouse must have space before the portal can resolve.
	bool bAwaitingRewardStorage = false;
};

struct FRaidDirectMove
{
	TWeakObjectPtr<APageCharacter> Raider;
	FVector Destination = FVector::ZeroVector;
};

/** Spawns and tracks the hostile wave released when an ignored portal raids the settlement. */
UCLASS()
class AEIDOS_API UWS_RaidDirector : public UWorldSubsystem, public ISimSystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void SimPlan_Implementation(USimCommandBuffer* CommandBuffer, float FixedDeltaSeconds) override;
	virtual void SimCommit_Implementation(USimCommandBuffer* CommandBuffer, float FixedDeltaSeconds) override;
	virtual void SimPost_Implementation(float FixedDeltaSeconds) override;
	virtual int32 GetSimOrder_Implementation() const override { return 410; }

	void StartRaid(const FPortalState& Portal);

	UFUNCTION(BlueprintPure, Category="Raid")
	bool IsRaidActive(int32 PortalId) const;

	/** True while any raid still owns runtime raiders or wave state. */
	UFUNCTION(BlueprintPure, Category="Raid")
	bool HasAnyActiveRaid() const { return !ActiveRaids.IsEmpty(); }

	UFUNCTION(BlueprintPure, Category="Raid")
	bool GetRaidWaveProgress(int32 PortalId, int32& OutCurrentWave, int32& OutTotalWaves) const;

	UFUNCTION(BlueprintPure, Category="Raid")
	bool IsSettlementDefeated() const { return bSettlementDefeated; }

	UPROPERTY(BlueprintAssignable, Category="Raid")
	FOnSettlementDefeated OnSettlementDefeated;

private:
	UClass* ResolveRaiderClass() const;
	void SpawnNextWave(FRaidRuntime& Raid);
	void CompleteRaid(int32 PortalId);
	bool HasActiveRaider(const FRaidRuntime& Raid) const;
	ASettlementCoreActor* FindSettlementCore() const;
	void MoveRaidersTowardCore(FRaidRuntime& Raid, ASettlementCoreActor* SettlementCore, float FixedDeltaSeconds);
	void ResolveSettlementDefeat();

	UFUNCTION()
	void HandleSettlementCoreDestroyed(ASettlementCoreActor* DestroyedCore);

	UPROPERTY(Transient)
	TMap<int32, FRaidRuntime> ActiveRaids;

	UPROPERTY(Transient)
	TArray<int32> PlannedResolvedRaids;
	TArray<FRaidDirectMove> PlannedDirectMoves;

	float PlannedCoreDamage = 0.f;
	bool bSettlementDefeated = false;
	bool bWarnedMissingSettlementCore = false;
	float CoreAttackRangeCm = 180.f;
	float RaiderCoreDamagePerSecond = 12.f;
	// Keep raids readable and give the player time to react.
	float RaiderMoveSpeedCmPerSecond = 180.f;
	float RepathIntervalSeconds = 0.5f;

	/** Successful settlement defenses award a real warehouse item, not a legacy abstract resource. */
	UPROPERTY(EditDefaultsOnly, Category="Raid|Rewards")
	FName RaidRewardItemId = TEXT("PortalShard");

	UPROPERTY(EditDefaultsOnly, Category="Raid|Rewards", meta=(ClampMin="0"))
	int32 RaidRewardPerDifficulty = 1;
};
