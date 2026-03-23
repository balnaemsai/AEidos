// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Save/SaveGameParticipant.h"
#include "Save/SaveGameSchema.h"
#include "Subsystems/WorldSubsystem.h"
#include "Simulation/SimSystem.h"
#include "WorkTypes.h"
#include "WorkDefinitionRow.h"
#include "WS_Work.generated.h"

class USimCommandBuffer;

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

	UFUNCTION(BlueprintCallable)
	bool CancelWorkRequest(int32 RequestId);

	virtual void WriteToSnapshot_Implementation(FEidosWorldSnapshot& InOutSnapshot) const override;
	virtual void ApplySnapshot_Implementation(const FEidosWorldSnapshot& Snapshot) override;

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

	// PageId -> Jobs
	UPROPERTY()
	TMap<int32, FJobArray> PageJobs;

	// Cached subsystem pointers (interfaces)
	UPROPERTY() UObject* EconomyObj = nullptr;
	UPROPERTY() UObject* PopulationObj = nullptr;

	// ---- helpers ----
	const FWorkDefinitionRow* FindDef(FName WorkId) const;
	bool IsRequestSatisfied(const FWorkRequest& Req) const;

	void TrySpawnInstancesFromQueue();
	void UpdateAssignments(float FixedDeltaSeconds);
	void ProgressInstances(float FixedDeltaSeconds);
	void HandleInstanceCompleted(const FWorkInstance& Inst);

	FVector ResolveSiteLocationForWork(const FWorkDefinitionRow& Def) const;
	
};
