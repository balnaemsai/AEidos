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
	/** Starts a clean session and discards the transient new-game snapshot from any earlier run. */
	void StartNewGame(const FString& MapNameHint = TEXT(""));
	
	void ApplyPendingSnapshotToWorld(UWorld& World);


	bool SaveToSlot(UWorld& World, const FString& SlotName, int32 UserIndex = 0);
	bool LoadFromSlotToPending(const FString& SlotName, int32 UserIndex = 0);

	/**
	 * The current snapshot format intentionally excludes streamed dungeon and
	 * active raid runtime actors. Rejecting those saves is safer than creating
	 * a slot that cannot be restored faithfully.
	 */
	bool CanSaveWorld(const UWorld& World, FString& OutReason) const;

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
