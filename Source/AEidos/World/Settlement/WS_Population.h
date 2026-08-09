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

	const TArray<TWeakObjectPtr<APageCharacter>>& GetOwnedPages() const;

	/** Returns Pages currently held as captives. Captives are not controllable or assigned to work. */
	void GetCaptivePages(TArray<APageCharacter*>& OutCaptives) const;

	APageCharacter* FindCaptiveById(int32 PageId) const;
	bool CaptureHostilePage(APageCharacter* TargetPage);
	bool RecruitCaptivePage(int32 PageId, FString& OutReason);

	UFUNCTION(BlueprintPure, Category="Population|Capacity")
	int32 GetCurrentPageCount() const;

	UFUNCTION(BlueprintPure, Category="Population|Capacity")
	int32 GetPageCapacity() const;

	UFUNCTION(BlueprintPure, Category="Population|Capacity")
	bool IsOverCapacity() const;

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

	UPROPERTY(EditDefaultsOnly, Category="Population|Test", meta=(ClampMin="1"))
	int32 InitialTestPageCount = 2;

	UPROPERTY(EditDefaultsOnly, Category="Population|Capacity", meta=(ClampMin="0"))
	int32 BasePageCapacity = 2;

	UPROPERTY(EditDefaultsOnly, Category="Population|Capacity", meta=(ClampMin="0.01", ClampMax="1.0"))
	float OverCapacityWorkRateMultiplier = 0.70f;

	UPROPERTY(EditDefaultsOnly, Category="Population|Test")
	FVector TestSpawnOffsetPerPage = FVector(150.f, 0.f, 0.f);

	// Give the first freshly spawned test Page a visible inventory entry for UI verification.
	UPROPERTY(EditDefaultsOnly, Category="Population|Test")
	bool bGiveStarterTestItem = true;

	UPROPERTY(EditDefaultsOnly, Category="Population|Test", meta=(EditCondition="bGiveStarterTestItem"))
	FName StarterTestItemId = TEXT("Ration");

	UPROPERTY(EditDefaultsOnly, Category="Population|Test", meta=(ClampMin="1", EditCondition="bGiveStarterTestItem"))
	int32 StarterTestItemQuantity = 1;

	UPROPERTY(EditDefaultsOnly, Category="Population|Test")
	bool bGiveStarterBlockItems = true;

	UPROPERTY(EditDefaultsOnly, Category="Population|Test", meta=(EditCondition="bGiveStarterBlockItems"))
	FName StarterBlockItemId = TEXT("StoneBlock");

	UPROPERTY(EditDefaultsOnly, Category="Population|Test", meta=(ClampMin="1", EditCondition="bGiveStarterBlockItems"))
	int32 StarterBlockItemQuantity = 8;

	UPROPERTY(EditDefaultsOnly, Category="Population|Test")
	bool bEquipStarterTestTool = true;

	UPROPERTY(EditDefaultsOnly, Category="Population|Test", meta=(EditCondition="bEquipStarterTestTool"))
	FName StarterTestToolItemId = TEXT("TestPickaxe");

	// Additional test equipment is granted to the first Page inventory, not auto-equipped.
	UPROPERTY(EditDefaultsOnly, Category="Population|Test")
	bool bGiveStarterTestEquipment = true;

	UPROPERTY(EditDefaultsOnly, Category="Population|Test", meta=(EditCondition="bGiveStarterTestEquipment"))
	TArray<FName> StarterTestEquipmentItemIds = { TEXT("TestKnife"), TEXT("TestHelmet"), TEXT("TestJacket"), TEXT("TestPants"), TEXT("TestBoots") };

private:
	
	void RebuildCacheIfNeeded();
	void MarkCacheDirty();
	void RefreshSettlementCapacityState();
	int32 EnsurePageEntityId(APageCharacter* Page);
	APageCharacter* FindPageById(int32 PageId) const;
	void ResetPageRuntimeState(APageCharacter* Page) const;

	UPROPERTY()
	TArray<TWeakObjectPtr<APageCharacter>> CachedPages;

	TMap<int32, TWeakObjectPtr<APageCharacter>> CachedPagesById;

	bool bCacheDirty = true;
	int32 NextPageId = 1;
	int32 CachedPageCapacity = 0;
	bool bCachedOverCapacity = false;

	// 이번 틱에 적용할 델타(캐시와 같은 인덱스)
	UPROPERTY()
	TArray<struct FPageStatsDelta> PlannedDeltas;
	
};
