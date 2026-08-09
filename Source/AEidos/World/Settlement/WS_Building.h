#pragma once

#include "CoreMinimal.h"
#include "Save/SaveGameParticipant.h"
#include "Simulation/SimSystem.h"
#include "Subsystems/WorldSubsystem.h"
#include "Data/Definitions/BuildingDefinitionRow.h"
#include "WS_Building.generated.h"

class AActor;
class UGIS_DataRegistry;
class USimCommandBuffer;
class UWS_Work;

USTRUCT(BlueprintType)
struct FConstructionSiteState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	int32 SiteId = 0;

	UPROPERTY(BlueprintReadWrite)
	FName BuildingId;

	UPROPERTY(BlueprintReadWrite)
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite)
	float YawDeg = 0.f;

	UPROPERTY(BlueprintReadWrite)
	int32 WorkRequestId = 0;

	UPROPERTY(BlueprintReadWrite)
	EConstructionSiteLifecycle State = EConstructionSiteLifecycle::Queued;

	UPROPERTY()
	TWeakObjectPtr<AActor> SiteActor;

	UPROPERTY()
	TWeakObjectPtr<AActor> FinalActor;
};

UCLASS()
class AEIDOS_API UWS_Building
	: public UWorldSubsystem
	, public ISimSystem
	, public ISaveGameParticipant
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	virtual void SimPlan_Implementation(USimCommandBuffer* CommandBuffer, float FixedDeltaSeconds) override;
	virtual void SimCommit_Implementation(USimCommandBuffer* CommandBuffer, float FixedDeltaSeconds) override;
	virtual void SimPost_Implementation(float FixedDeltaSeconds) override;
	virtual int32 GetSimOrder_Implementation() const override { return 30; }

	virtual void WriteToSnapshot_Implementation(FEidosWorldSnapshot& OutSnapshot) const override;
	virtual void ApplySnapshot_Implementation(const FEidosWorldSnapshot& Snapshot) override;

	UFUNCTION(BlueprintCallable)
	bool ValidatePlacement(FName BuildingId, FVector Location, float YawDeg, FString& OutReason) const;

	UFUNCTION(BlueprintCallable)
	int32 RequestBuild(FName BuildingId, FVector Location, float YawDeg);

	UFUNCTION(BlueprintCallable)
	const TArray<FConstructionSiteState>& GetConstructionSites() const { return ConstructionSites; }

	void GetCompletedBuildingIds(TArray<FName>& OutBuildingIds) const;

	/** Total Page capacity supplied by completed buildings only. */
	int32 GetCompletedPageCapacity() const;

	UFUNCTION(BlueprintCallable)
	void LoadBuildingDefs();

	void RegisterAutoWorksForBuilding(FName BuildingId, const FVector& BuildingLocation);

private:
	const FBuildingDefinitionRow* FindBuildingDef(FName BuildingId) const;
	FConstructionSiteState* FindConstructionSiteById(int32 SiteId);
	FConstructionSiteState* FindConstructionSiteByRequestId(int32 WorkRequestId);
	bool IntersectsAnyPlacedOrConstruction(const FBuildingDefinitionRow& Def, FVector Location) const;
	int32 CreateConstructionSite(FName BuildingId, FVector Location, float YawDeg, int32 WorkRequestId);
	void FinalizeBuilding(int32 SiteId);
	void CleanupInvalidActors();
	void DestroyAllRuntimeActors();
	void RespawnActorsForSite(FConstructionSiteState& Site, bool bRegisterAutoWorks);
	void SpawnConstructionSiteActor(FConstructionSiteState& Site);
	void SpawnFinalBuildingActor(FConstructionSiteState& Site, bool bRegisterAutoWorks);
	void HandleWorkRequestStateChanged(int32 WorkRequestId, EWorkRequestLifecycleState NewState);

	UPROPERTY()
	TMap<FName, FBuildingDefinitionRow> BuildingDefs;

	UPROPERTY()
	TArray<FConstructionSiteState> ConstructionSites;

	UPROPERTY()
	TArray<int32> PendingFinalizeSiteIds;

	UPROPERTY()
	TArray<int32> PlannedFinalizeSiteIds;

	UPROPERTY()
	TObjectPtr<UWS_Work> WorkSubsystem = nullptr;

	int32 NextSiteId = 1;
};
