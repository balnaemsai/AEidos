// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Simulation/SimSystem.h" 
#include "WS_Population.generated.h"

class USimCommandBuffer;
class APageCharacter;

/**
 * 
 */
UCLASS()
class AEIDOS_API UWS_Population : public UWorldSubsystem, public ISimSystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	virtual int32 GetSimOrder_Implementation() const override;
	virtual void SimPlan_Implementation(USimCommandBuffer* CommandBuffer, float FixedDeltaSeconds) override;
	virtual void SimCommit_Implementation(USimCommandBuffer* CommandBuffer, float FixedDeltaSeconds) override;
	virtual void SimPost_Implementation(float FixedDeltaSeconds) override;

	const TArray<TWeakObjectPtr<APageCharacter>>& GetOwnedPages() const { return CachedPages; }

	void EnsureTestPageSpawned();

protected:
	UPROPERTY(EditDefaultsOnly, Category="Population|Test")
	bool bSpawnTestPage = true;

	UPROPERTY(EditDefaultsOnly, Category="Population|Test")
	TSubclassOf<APageCharacter> TestPageClass;

	UPROPERTY(EditDefaultsOnly, Category="Population|Test")
	FVector TestSpawnLocation = FVector(0.f, 0.f, 200.f);

private:
	
	void RebuildCacheIfNeeded();

	UPROPERTY()
	TArray<TWeakObjectPtr<APageCharacter>> CachedPages;

	bool bCacheDirty = true;

	// 이번 틱에 적용할 델타(캐시와 같은 인덱스)
	UPROPERTY()
	TArray<struct FPageStatsDelta> PlannedDeltas;
	
};
