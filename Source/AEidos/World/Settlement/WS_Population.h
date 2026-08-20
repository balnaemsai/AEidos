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

/** Runtime-only emergency assignment used to save a downed friendly Page. */
USTRUCT()
struct FEmergencyRescueOperation
{
	GENERATED_BODY()

	UPROPERTY()
	int32 DownedPageId = INDEX_NONE;

	UPROPERTY()
	int32 RescuerPageId = INDEX_NONE;

	UPROPERTY()
	int32 InstanceId = INDEX_NONE;

	UPROPERTY()
	float RemainingWorkSeconds = 0.f;
};

/** Runtime-only targeted work that turns a captive's resistance into recruitment progress. */
USTRUCT()
struct FCaptiveRecruitmentOperation
{
	GENERATED_BODY()

	UPROPERTY()
	int32 CaptivePageId = INDEX_NONE;

	UPROPERTY()
	int32 RecruiterPageId = INDEX_NONE;

	UPROPERTY()
	int32 InstanceId = INDEX_NONE;
};

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
	bool ToggleCaptiveRecruitment(int32 PageId, FString& OutReason);
	bool IsCaptiveRecruitmentRequested(int32 PageId) const;
	bool IsCaptiveRecruitmentActive(int32 PageId) const;
	int32 GetCaptiveRecruiterPageId(int32 PageId) const;
	bool RescueDownedPage(APageCharacter* Rescuer, APageCharacter* DownedPage, FString& OutReason);

	/** Toggle a friendly Page in the expedition roster used by settlement portals. */
	bool TogglePageExpeditionRoster(int32 PageId, FString& OutReason);
	bool IsPageInExpeditionRoster(int32 PageId) const;
	void GetReadyExpeditionPages(TArray<APageCharacter*>& OutPages) const;
	int32 GetExpeditionRosterCount() const;
	int32 GetReadyExpeditionRosterCount() const;

	UFUNCTION(BlueprintPure, Category="Population|Capacity")
	int32 GetCurrentPageCount() const;

	UFUNCTION(BlueprintPure, Category="Population|Capacity")
	int32 GetPageCapacity() const;

	UFUNCTION(BlueprintPure, Category="Population|Capacity")
	bool IsOverCapacity() const;

	UFUNCTION(BlueprintPure, Category="Population|Captives")
	int32 GetCurrentCaptiveCount() const;

	UFUNCTION(BlueprintPure, Category="Population|Captives")
	int32 GetCaptiveCapacity() const;

	UFUNCTION(BlueprintPure, Category="Population|Captives")
	bool IsCaptiveCapacityFull() const;

	void EnsureTestPageSpawned();

	//PopulationAccess 구현

	virtual TArray<int32> GetAllPageIds_Implementation() const override;
	virtual AActor* GetPageActor_Implementation(int32 PageId) override;
	virtual bool IsPageAvailable_Implementation(int32 PageId) const override;
	virtual int32 GetPageWorkPriority_Implementation(int32 PageId, EWorkCategory WorkCategory) const override;
	virtual bool IsPageAssignedToWork_Implementation(int32 PageId, int32 InstanceId) const override;
	virtual float ComputeWorkRateMultiplier_Implementation(int32 PageId, FName SkillId) const override;
	virtual void AwardWorkSkillXP_Implementation(int32 PageId, FName SkillId, float XPPerSecond, float FixedDeltaSeconds, float XPFactor) override;
	virtual void ApplyWorkCompletionEffects_Implementation(int32 PageId, FName WorkId) override;
	virtual void AssignPageToWork_Implementation(int32 PageId, int32 InstanceId, FName WorkId, FVector WorkLocation, int32 Priority, bool bTeleportToWorkSite) override;
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

	UPROPERTY(EditDefaultsOnly, Category="Population|Captives", meta=(ClampMin="0"))
	int32 BaseCaptiveCapacity = 0;

	UPROPERTY(EditDefaultsOnly, Category="Population|Captives", meta=(ClampMin="1.0"))
	float InitialCaptiveResistance = 100.f;

	UPROPERTY(EditDefaultsOnly, Category="Population|Captives", meta=(ClampMin="0.01"))
	float CaptiveRecruitmentResistancePerSecond = 5.f;

	UPROPERTY(EditDefaultsOnly, Category="Population|Rescue", meta=(ClampMin="1.0"))
	float RescueRangeCm = 220.f;

	UPROPERTY(EditDefaultsOnly, Category="Population|Rescue", meta=(ClampMin="0.01", ClampMax="1.0"))
	float RescueHealthFraction = 0.25f;

	/** Safe settlement rescues are automatic. Dungeon and combat rescues remain manual. */
	UPROPERTY(EditDefaultsOnly, Category="Population|Rescue")
	bool bEnableAutomaticSettlementRescue = true;

	/** Time spent performing a safe automatic rescue after the rescuer reaches the target. */
	UPROPERTY(EditDefaultsOnly, Category="Population|Rescue", meta=(ClampMin="0.1"))
	float AutomaticRescueWorkSeconds = 3.f;

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
	void PlanAutomaticSettlementRescues(float FixedDeltaSeconds);
	void PlanCaptiveRecruitment(float FixedDeltaSeconds);
	bool IsEligibleAutomaticRescuer(const APageCharacter* Page) const;
	bool IsSafeAutomaticRescueTarget(const APageCharacter* Page) const;
	bool IsEligibleCaptiveRecruiter(const APageCharacter* Page) const;
	bool IsRecruitableCaptive(const APageCharacter* Page) const;
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
	int32 CachedCaptiveCapacity = 0;

	/** Saved Page IDs; invalid members are retained but skipped until ready again. */
	TSet<int32> ExpeditionRosterPageIds;

	// 이번 틱에 적용할 델타(캐시와 같은 인덱스)
	UPROPERTY()
	TArray<struct FPageStatsDelta> PlannedDeltas;

	UPROPERTY()
	TMap<int32, FEmergencyRescueOperation> ActiveEmergencyRescues;

	UPROPERTY()
	TArray<FEmergencyRescueOperation> PlannedEmergencyRescues;

	UPROPERTY()
	TArray<FEmergencyRescueOperation> PlannedCompletedEmergencyRescues;

	UPROPERTY()
	TArray<int32> PlannedCancelledEmergencyRescueTargetIds;

	int32 NextEmergencyRescueInstanceId = -1;

	TSet<int32> RequestedCaptiveRecruitmentIds;

	UPROPERTY()
	TMap<int32, FCaptiveRecruitmentOperation> ActiveCaptiveRecruitments;

	UPROPERTY()
	TArray<FCaptiveRecruitmentOperation> PlannedCaptiveRecruitments;

	UPROPERTY()
	TArray<FCaptiveRecruitmentOperation> PlannedCompletedCaptiveRecruitments;

	UPROPERTY()
	TArray<int32> PlannedCancelledCaptiveRecruitmentIds;

	int32 NextCaptiveRecruitmentInstanceId = -100000;
	
};
