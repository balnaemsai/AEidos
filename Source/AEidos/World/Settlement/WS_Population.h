// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EidosAccessInterface.h"
#include "Save/SaveGameParticipant.h"
#include "Subsystems/WorldSubsystem.h"
#include "Simulation/SimSystem.h" 
#include "WS_Population.generated.h"

class USimCommandBuffer;
class APageCharacter;

/**
 * 
 */
UCLASS()
class AEIDOS_API UWS_Population : public UWorldSubsystem, public ISimSystem, public IEidosPopulationAccess, public ISaveGameParticipant
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	//Simsystem 구현
	
	virtual int32 GetSimOrder_Implementation() const override;
	virtual void SimPlan_Implementation(USimCommandBuffer* CommandBuffer, float FixedDeltaSeconds) override;
	virtual void SimCommit_Implementation(USimCommandBuffer* CommandBuffer, float FixedDeltaSeconds) override;
	virtual void SimPost_Implementation(float FixedDeltaSeconds) override;

	const TArray<TWeakObjectPtr<APageCharacter>>& GetOwnedPages() const { return CachedPages; }

	void EnsureTestPageSpawned();

	//PopulationAccess 구현

	virtual TArray<int32> GetAllPageIds_Implementation() const override;
	virtual AActor* GetPageActor_Implementation(int32 PageId) override;
	virtual bool IsPageAvailable_Implementation(int32 PageId) const override;
	virtual float ComputeWorkRateMultiplier_Implementation(int32 PageId, FName WorkId) const override;
	virtual void ApplyWorkCompletionEffects_Implementation(int32 PageId, FName WorkId) override;
	virtual void AssignPageToWork_Implementation(int32 PageId, int32 InstanceId, FName WorkId, FVector WorkLocation, int32 Priority) override;
	virtual void ClearPageWorkAssignment_Implementation(int32 PageId, int32 InstanceId) override;

	virtual void WriteToSnapshot_Implementation(FEidosWorldSnapshot& InOutSnapshot) const override;
	virtual void ApplySnapshot_Implementation(const FEidosWorldSnapshot& Snapshot) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category="Population|Test")
	bool bSpawnTestPage = true;

	UPROPERTY(EditDefaultsOnly, Category="Population|Test")
	TSubclassOf<APageCharacter> TestPageClass;

	UPROPERTY(EditDefaultsOnly, Category="Population|Test")
	FVector TestSpawnLocation = FVector(0.f, 0.f, 200.f);

private:
	
	void RebuildCacheIfNeeded();
	int32 EnsurePageEntityId(APageCharacter* Page);
	APageCharacter* FindPageById(int32 PageId) const;
	void ResetPageRuntimeState(APageCharacter* Page) const;

	UPROPERTY()
	TArray<TWeakObjectPtr<APageCharacter>> CachedPages;

	TMap<int32, TWeakObjectPtr<APageCharacter>> CachedPagesById;

	bool bCacheDirty = true;
	int32 NextPageId = 1;

	// 이번 틱에 적용할 델타(캐시와 같은 인덱스)
	UPROPERTY()
	TArray<struct FPageStatsDelta> PlannedDeltas;
	
};
