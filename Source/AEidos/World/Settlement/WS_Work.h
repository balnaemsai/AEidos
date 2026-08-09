// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Save/SaveGameParticipant.h"
#include "Save/SaveGameSchema.h"
#include "Subsystems/WorldSubsystem.h"
#include "Simulation/SimSystem.h"
#include "Core/Types/WorkTypes.h"
#include "Data/Definitions/WorkDefinitionRow.h"
#include "WS_Work.generated.h"

class USimCommandBuffer;
class UWS_ItemStorage;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnWorkRequestStateChanged, int32, EWorkRequestLifecycleState);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnWorkCompleted, int32, FName);

USTRUCT()
struct FPlannedPageAssignment
{
	GENERATED_BODY()

	UPROPERTY()
	int32 PageId = INDEX_NONE;

	UPROPERTY()
	int32 InstanceId = INDEX_NONE;

	UPROPERTY()
	FName WorkId;

	UPROPERTY()
	FVector WorkLocation = FVector::ZeroVector;

	UPROPERTY()
	int32 Priority = 0;
};

/**
 * 
 */

UCLASS()
class AEIDOS_API UWS_Work : public UWorldSubsystem, public ISimSystem, public ISaveGameParticipant
{
	GENERATED_BODY()

public:
	virtual void SimPlan_Implementation(USimCommandBuffer* Cmd, float FixedDeltaSeconds) override;
	virtual void SimCommit_Implementation(USimCommandBuffer* Cmd, float FixedDeltaSeconds) override;
	virtual void SimPost_Implementation(float FixedDeltaSeconds) override {}
	virtual int32 GetSimOrder_Implementation() const override { return 50; }

	UFUNCTION(BlueprintCallable)
	int32 AddWorkRequest(const FWorkRequest& InReq);

	/** Convenience entry point for UI: queues the specified DT_Work row a number of times. */
	UFUNCTION(BlueprintCallable)
	int32 QueueWorkById(FName WorkId, int32 Quantity = 1, int32 Priority = 0);

	UFUNCTION(BlueprintPure)
	TArray<FWorkOrderView> GetCraftableWorkOrders() const;
	bool GetWorkOrderView(FName WorkId, FWorkOrderView& OutView) const;
	void GetOutstandingRequestIdsForWork(FName WorkId, TArray<int32>& OutRequestIds) const;

	UFUNCTION(BlueprintCallable)
	bool CancelWorkRequest(int32 RequestId);

	virtual void WriteToSnapshot_Implementation(FEidosWorldSnapshot& InOutSnapshot) const override;
	virtual void ApplySnapshot_Implementation(const FEidosWorldSnapshot& Snapshot) override;

	UFUNCTION(BlueprintCallable)
	bool HasQueuedRequest(int32 RequestId) const;

	UFUNCTION(BlueprintCallable)
	bool HasActiveInstanceForRequest(int32 RequestId) const;

	UFUNCTION(BlueprintCallable)
	void LoadWorkDefs();

	FOnWorkRequestStateChanged OnWorkRequestStateChanged;
	FOnWorkCompleted OnWorkCompleted;

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
private:
	UPROPERTY()
	FWorkProducer Producer;

	float ProductionAccumulator = 0.0f;

	int32 PlannedDeltaInt = 0;

	UPROPERTY() TMap<FName, FWorkDefinitionRow> WorkDefs;

	// Queue
	UPROPERTY() TArray<FWorkRequest> Queue;
	int32 NextRequestId = 1;

	// Active instances
	UPROPERTY() TArray<FWorkInstance> ActiveInstances;
	int32 NextInstanceId = 1;

	// 이번 틱 Commit에서 추가할 인스턴스
	UPROPERTY()
	TArray<FWorkInstance> PlannedNewInstances;

	// 이번 틱 Commit에서 완료 처리할 인스턴스
	UPROPERTY()
	TArray<FWorkInstance> PlannedCompletedInstances;

	UPROPERTY()
	TArray<FPlannedPageAssignment> PlannedPageAssignments;

	// Cached subsystem pointers (interfaces)
	UPROPERTY() UObject* EconomyObj = nullptr;
	UPROPERTY() UObject* PopulationObj = nullptr;
	UPROPERTY() TObjectPtr<UWS_ItemStorage> ItemStorage = nullptr;

	// ---- helpers ----
	const FWorkDefinitionRow* FindDef(FName WorkId) const;
	bool IsRequestSatisfied(const FWorkRequest& Req) const;
	bool CanAffordWorkCosts(const TArray<FWorkCost>& Costs) const;
	/** Includes costs reserved by pending Count-mode orders, preventing over-queuing. */
	bool CanReserveWorkCosts(const TArray<FWorkCost>& Costs, int32 Quantity) const;
	bool CanStoreWorkRewards(const TArray<FWorkReward>& Rewards) const;
	void ConsumeWorkCosts(const TArray<FWorkCost>& Costs);
	void GrantWorkRewards(const TArray<FWorkReward>& Rewards);

	void TrySpawnInstancesFromQueue();
	void UpdateAssignments(float FixedDeltaSeconds);
	void ProgressInstances(float FixedDeltaSeconds);
	void HandleInstanceCompleted(const FWorkInstance& Inst);
	void BroadcastRequestState(int32 RequestId, EWorkRequestLifecycleState NewState);

	FVector ResolveSiteLocationForWork(const FWorkDefinitionRow& Def) const;
	
	
};
