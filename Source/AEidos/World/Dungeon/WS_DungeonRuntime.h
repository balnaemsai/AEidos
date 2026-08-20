#pragma once

#include "CoreMinimal.h"
#include "Core/Types/DungeonTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "WS_DungeonRuntime.generated.h"

class APageCharacter;
class AActor;
class ADungeonCoreActor;
class ADungeonReturnPortalActor;
class ATerritoryChunkActor;
class UDungeonSettlementPreset;
class ULevel;
class ULevelStreamingDynamic;
class UWorld;
struct FPortalState;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnDungeonCoreDestroyedForScenario, int32 /* PortalId */);

USTRUCT()
struct FDungeonSessionRuntime
{
	GENERATED_BODY()

	UPROPERTY()
	int32 PortalId = INDEX_NONE;

	UPROPERTY()
	FTransform ReturnTransform = FTransform::Identity;

	/** Stable entry anchor used by every Page that joins this expedition. */
	UPROPERTY()
	FTransform EntryTransform = FTransform::Identity;

	UPROPERTY()
	bool bPageTransferred = false;

	UPROPERTY()
	float SettlementValueAtSpawn = 0.f;

	UPROPERTY()
	float DungeonDifficulty = 1.f;

	UPROPERTY()
	TArray<FDungeonAttributeWeight> DungeonAttributes;

	/** The layout selected when this expedition began. */
	UPROPERTY()
	TObjectPtr<UDungeonSettlementPreset> SettlementPreset = nullptr;

	UPROPERTY()
	FName SettlementPresetId = NAME_None;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> SpawnedActors;

	TWeakObjectPtr<ADungeonCoreActor> DungeonCore;
	TWeakObjectPtr<ADungeonReturnPortalActor> ReturnPortal;
	TArray<TWeakObjectPtr<APageCharacter>> DungeonPages;
	TWeakObjectPtr<ULevelStreamingDynamic> StreamingLevel;
	bool bCoreDestroyed = false;
	double CollapseEndTimeSeconds = 0.0;
};

UCLASS()
class AEIDOS_API UWS_DungeonRuntime : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UWS_DungeonRuntime();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category="Dungeon")
	bool EnterDungeonForPortal(int32 PortalId, APageCharacter* EnteringPage);

	/** C++ portal path for entering a single shared dungeon with a prepared group. */
	bool EnterDungeonForPortal(int32 PortalId, const TArray<APageCharacter*>& EnteringPages);

	UFUNCTION(BlueprintCallable, Category="Dungeon")
	bool ReturnPageFromActiveDungeon(APageCharacter* ReturningPage);

	UFUNCTION(BlueprintPure, Category="Dungeon")
	bool HasActiveDungeon() const;

	UFUNCTION(BlueprintPure, Category="Dungeon")
	bool IsPageInActiveDungeon(const APageCharacter* Page) const;

	UFUNCTION(BlueprintPure, Category="Dungeon")
	bool IsActiveDungeonForPortal(int32 PortalId) const;

	UFUNCTION(BlueprintPure, Category="Dungeon")
	bool IsDungeonCollapseActive() const;

	UFUNCTION(BlueprintPure, Category="Dungeon")
	float GetDungeonCollapseRemainingSeconds() const;

	UFUNCTION(BlueprintPure, Category="Dungeon")
	int32 GetActiveDungeonPageCount() const;

	FOnDungeonCoreDestroyedForScenario OnDungeonCoreDestroyedForScenario;

private:
	UFUNCTION()
	void HandleActiveDungeonLevelShown();

	UFUNCTION()
	void HandleDungeonCoreDestroyed(ADungeonCoreActor* DestroyedCore);

	void HandleDungeonCollapseExpired();

	FTransform ResolveDungeonEntryTransform(ULevel* LoadedLevel) const;
	UDungeonSettlementPreset* ResolveSettlementPresetForPortal(const FPortalState& PortalState) const;
	bool IsPresetCompatibleWithAttributes(const UDungeonSettlementPreset& Preset, const TArray<FDungeonAttributeWeight>& Attributes) const;
	FTransform MakeDungeonWorldTransform(const FTransform& LocalTransform) const;
	void SpawnPresetLayoutIntoDungeon(ULevel* LoadedLevel);
	void SpawnDungeonChunks(ULevel* LoadedLevel, const UDungeonSettlementPreset* Preset);
	void SpawnDungeonBuildings(ULevel* LoadedLevel, const UDungeonSettlementPreset* Preset);
	void SpawnDungeonCore(ULevel* LoadedLevel, const UDungeonSettlementPreset* Preset);
	void SpawnDungeonEnemies(ULevel* LoadedLevel, const UDungeonSettlementPreset* Preset);
	void SpawnDungeonWorldBlocks(ULevel* LoadedLevel, const UDungeonSettlementPreset* Preset);
	void SpawnDungeonWorldItems(ULevel* LoadedLevel, const UDungeonSettlementPreset* Preset);
	void SpawnCoreShardWorldItems(ADungeonCoreActor* DestroyedCore, ULevel* LoadedLevel);
	void StartDungeonCollapse(const FTransform& CoreTransform, ULevel* LoadedLevel);
	int32 ReturnCaptivesFromActiveDungeon();
	void DestroyPagesStillInDungeon();
	void EndActiveDungeonSession();
	void ResetActiveSession();
	void MovePageIntoDungeon(APageCharacter* Page, const FTransform& EntryTransform);
	bool AddPageToActiveDungeon(APageCharacter* Page);
	FTransform GetExpeditionFormationTransform(const FTransform& AnchorTransform, const APageCharacter* Page) const;
	bool CanJoinActiveExpedition(const APageCharacter* Page) const;

	UPROPERTY(EditDefaultsOnly, Category="Dungeon")
	TSoftObjectPtr<UWorld> DefaultDungeonLevel;

	UPROPERTY(EditDefaultsOnly, Category="Dungeon")
	TSoftObjectPtr<UDungeonSettlementPreset> DefaultSettlementPreset;

	UPROPERTY(EditDefaultsOnly, Category="Dungeon")
	TSoftClassPtr<APageCharacter> DefaultEnemyPageClass;

	UPROPERTY(EditDefaultsOnly, Category="Dungeon")
	TSubclassOf<ADungeonCoreActor> DefaultDungeonCoreClass;

	UPROPERTY(EditDefaultsOnly, Category="Dungeon|Collapse", meta=(ClampMin="1.0"))
	float DungeonCollapseDurationSeconds = 120.f;

	UPROPERTY(EditDefaultsOnly, Category="Dungeon|Collapse")
	TSubclassOf<ADungeonReturnPortalActor> DungeonReturnPortalClass;

	UPROPERTY(EditDefaultsOnly, Category="Dungeon")
	FTransform DungeonLevelTransform = FTransform(FRotator::ZeroRotator, FVector(500000.f, 0.f, 0.f));

	UPROPERTY(EditDefaultsOnly, Category="Dungeon")
	FName DungeonEntryTag = TEXT("DungeonEntry");

	UPROPERTY(Transient)
	FDungeonSessionRuntime ActiveSession;

	FTimerHandle DungeonCollapseTimerHandle;
};
