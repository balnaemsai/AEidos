// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WS_DungeonRuntime.generated.h"

class APageCharacter;
class AActor;
class ATerritoryChunkActor;
class UDungeonSettlementPreset;
class ULevel;
class ULevelStreamingDynamic;
class UWorld;

USTRUCT()
struct FDungeonSessionRuntime
{
	GENERATED_BODY()

	UPROPERTY()
	int32 PortalId = INDEX_NONE;

	UPROPERTY()
	int32 PageEntityId = INDEX_NONE;

	UPROPERTY()
	FTransform ReturnTransform = FTransform::Identity;

	UPROPERTY()
	bool bPageTransferred = false;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> SpawnedActors;

	TWeakObjectPtr<APageCharacter> OccupyingPage;
	TWeakObjectPtr<ULevelStreamingDynamic> StreamingLevel;
};

UCLASS()
class AEIDOS_API UWS_DungeonRuntime : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UWS_DungeonRuntime();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category="Dungeon")
	bool EnterDungeonForPortal(int32 PortalId, APageCharacter* EnteringPage);

	UFUNCTION(BlueprintPure, Category="Dungeon")
	bool HasActiveDungeon() const;

	UFUNCTION(BlueprintPure, Category="Dungeon")
	bool IsPageInActiveDungeon(const APageCharacter* Page) const;

private:
	UFUNCTION()
	void HandleActiveDungeonLevelShown();

	FTransform ResolveDungeonEntryTransform(ULevel* LoadedLevel) const;
	FTransform MakeDungeonWorldTransform(const FTransform& LocalTransform) const;
	void SpawnPresetLayoutIntoDungeon(ULevel* LoadedLevel);
	void SpawnDungeonChunks(ULevel* LoadedLevel, const UDungeonSettlementPreset* Preset);
	void SpawnDungeonBuildings(ULevel* LoadedLevel, const UDungeonSettlementPreset* Preset);
	void SpawnDungeonEnemies(ULevel* LoadedLevel, const UDungeonSettlementPreset* Preset);
	void ResetActiveSession();
	void MovePageIntoDungeon(APageCharacter* Page, const FTransform& EntryTransform);

	UPROPERTY(EditDefaultsOnly, Category="Dungeon")
	TSoftObjectPtr<UWorld> DefaultDungeonLevel;

	UPROPERTY(EditDefaultsOnly, Category="Dungeon")
	TSoftObjectPtr<UDungeonSettlementPreset> DefaultSettlementPreset;

	UPROPERTY(EditDefaultsOnly, Category="Dungeon")
	TSoftClassPtr<APageCharacter> DefaultEnemyPageClass;

	UPROPERTY(EditDefaultsOnly, Category="Dungeon")
	FTransform DungeonLevelTransform = FTransform(FRotator::ZeroRotator, FVector(500000.f, 0.f, 0.f));

	UPROPERTY(EditDefaultsOnly, Category="Dungeon")
	FName DungeonEntryTag = TEXT("DungeonEntry");

	UPROPERTY(Transient)
	FDungeonSessionRuntime ActiveSession;
};
