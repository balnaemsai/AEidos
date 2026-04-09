// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Simulation/SimSystem.h"
#include "Save/SaveGameParticipant.h"
#include "Core/Types/PortalTypes.h"
#include "WS_PortalDirector.generated.h"

class USimCommandBuffer;
class UGIS_DataRegistry;
class UWS_RaidDirector;
class APortalActor;
struct FPortalDefinitionRow;
struct FEidosWorldSnapshot;
struct FPortalState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPortalListChanged);

/**
 * 
 */
UCLASS()
class AEIDOS_API UWS_PortalDirector : public UWorldSubsystem, public ISimSystem, public ISaveGameParticipant
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// ISimSystem
	virtual void SimPlan_Implementation(USimCommandBuffer* CommandBuffer, float FixedDeltaSeconds) override;
	virtual void SimCommit_Implementation(USimCommandBuffer* CommandBuffer, float FixedDeltaSeconds) override;
	virtual void SimPost_Implementation(float FixedDeltaSeconds) override;
	virtual int32 GetSimOrder_Implementation() const override { return 400; }

	// SaveGameParticipant
	virtual void WriteToSnapshot_Implementation(FEidosWorldSnapshot& OutSnapshot) const override;
	virtual void ApplySnapshot_Implementation(const FEidosWorldSnapshot& Snapshot) override;

	UFUNCTION(BlueprintCallable)
	bool ValidateEntry(int32 PortalId) const;

	UFUNCTION(BlueprintCallable)
	bool RequestEnterPortal(int32 PortalId);

	UFUNCTION(BlueprintCallable)
	void OnDungeonCleared(int32 PortalId);

	UFUNCTION(BlueprintCallable)
	bool TryGetPortalState(int32 PortalId, FPortalState& OutState) const;

	UFUNCTION(BlueprintCallable)
	void SpawnPortalNow();

	UFUNCTION(BlueprintCallable)
	TArray<FPortalState> GetActivePortals() const;

	UPROPERTY(BlueprintAssignable)
	FOnPortalListChanged OnPortalListChanged;

private:
	UPROPERTY()
	TMap<int32, FPortalState> ActivePortals;

	UPROPERTY()
	TMap<int32, TWeakObjectPtr<AActor>> PortalActors;

	UPROPERTY()
	int32 NextPortalId = 1;

	UPROPERTY()
	float TimeSinceLastSpawn = 0.f;

	UPROPERTY()
	bool bDirty = false;

	UPROPERTY(Transient)
	TArray<FPortalState> PlannedSpawnPortals;

	UPROPERTY(Transient)
	TArray<int32> PlannedRemovePortals;

	UGIS_DataRegistry* GetRegistry() const;
	const FPortalDefinitionRow* GetPortalDef(FName PortalDefId) const;
	const FPortalDefinitionRow* GetDefaultPortalDef() const;

	void CheckSpawn(float FixedDeltaSeconds);
	void UpdatePortalTimer(float FixedDeltaSeconds);

	FPortalState MakePortalState(const FPortalDefinitionRow& Def) const;
	FVector ChoosePortalSpawnLocation(const FPortalDefinitionRow& Def) const;

	void SpawnPortalActorForState(const FPortalState& State);
	void EnsurePortalActorsFromState();
	void RemovePortalInternal(int32 PortalId);

	static FName Key_NextPortalId();
	static FName Key_TimeSinceLastSpawn();
	static FName Key_ActivePortals();

	static FString EncodePortalStates(const TMap<int32, FPortalState>& InStates);
	static void DecodePortalStates(const FString& Encoded, TArray<FPortalState>& OutStates);
	
};
