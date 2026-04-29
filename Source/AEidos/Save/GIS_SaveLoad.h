// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SaveGameSchema.h"
#include "GIS_SaveLoad.generated.h"

class UEidosSaveGame;

DECLARE_LOG_CATEGORY_EXTERN(LogSaveLoad, Log, All);

/**
 * 
 */
UCLASS()
class AEIDOS_API UGIS_SaveLoad : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	bool HasPendingSnapshot() const { return bHasPendingSnapshot; }
	void SetPendingSnapshot(const FEidosWorldSnapshot& InSnapshot);
	void ClearPendingSnapshot();
	
	void ApplyPendingSnapshotToWorld(UWorld& World);


	bool SaveToSlot(UWorld& World, const FString& SlotName, int32 UserIndex = 0);
	bool LoadFromSlotToPending(const FString& SlotName, int32 UserIndex = 0);

	void BuildNewGameSnapshotIfNeeded(const FString& MapNameHint = TEXT(""));

private:
	FEidosWorldSnapshot CaptureWorldSnapshot(UWorld& World) const;
	void DispatchApplySnapshot(UWorld& World, const FEidosWorldSnapshot& Snapshot);
	void GatherSaveParticipants(UWorld& World, TArray<UObject*>& OutParticipants) const;

private:
	UPROPERTY(Transient)
	bool bHasPendingSnapshot = false;

	UPROPERTY(Transient)
	FEidosWorldSnapshot PendingSnapshot;
	
	UPROPERTY(Transient)
	bool bHasNewGameSnapshot = false;

	UPROPERTY(Transient)
	FEidosWorldSnapshot NewGameSnapshot;
};
