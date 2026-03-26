#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Simulation/SimSystem.h"
#include "Save/SaveGameParticipant.h"
#include "Data/Definitions/BuildingDefinitionRow.h"
#include "WS_Building.generated.h"

class UGIS_DataRegistry;
class UWS_Work;
class AActor;
class USimCommandBuffer;

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

	// WS_Work에 넣은 건설 요청
	UPROPERTY(BlueprintReadWrite)
	int32 WorkRequestId = 0;

	// 공사장 Actor (런타임 캐시)
	UPROPERTY()
	TWeakObjectPtr<AActor> SiteActor;

	// 최종 건물 Actor (런타임 캐시)
	UPROPERTY()
	TWeakObjectPtr<AActor> FinalActor;

	UPROPERTY(BlueprintReadWrite)
	bool bCompleted = false;
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

	// ISimSystem
	virtual void SimPlan_Implementation(USimCommandBuffer* CommandBuffer, float FixedDeltaSeconds) override;
	virtual void SimCommit_Implementation(USimCommandBuffer* CommandBuffer, float FixedDeltaSeconds) override;
	virtual void SimPost_Implementation(float FixedDeltaSeconds) override;
	virtual int32 GetSimOrder_Implementation() const override { return 30; }

	// Save
	virtual void WriteToSnapshot_Implementation(FEidosWorldSnapshot& OutSnapshot) const override;
	virtual void ApplySnapshot_Implementation(const FEidosWorldSnapshot& Snapshot) override;

	UFUNCTION(BlueprintCallable)
	bool ValidatePlacement(FName BuildingId, FVector Location, float YawDeg, FString& OutReason) const;

	UFUNCTION(BlueprintCallable)
	int32 RequestBuild(FName BuildingId, FVector Location, float YawDeg);

	UFUNCTION(BlueprintCallable)
	const TArray<FConstructionSiteState>& GetConstructionSites() const { return ConstructionSites; }

private:
	const FBuildingDefinitionRow* FindBuildingDef(FName BuildingId) const;
	void LoadBuildingDefs();

	bool IntersectsAnyPlacedOrConstruction(const FBuildingDefinitionRow& Def, FVector Location) const;
	int32 CreateConstructionSite(FName BuildingId, FVector Location, float YawDeg, int32 WorkRequestId);
	void FinalizeBuilding(int32 SiteId);
	void CleanupInvalidActors();

	UPROPERTY()
	TMap<FName, FBuildingDefinitionRow> BuildingDefs;

	UPROPERTY()
	TArray<FConstructionSiteState> ConstructionSites;

	UPROPERTY()
	TArray<int32> PlannedCompletedSiteIds;

	UPROPERTY()
	TObjectPtr<UWS_Work> WorkSubsystem = nullptr;

	int32 NextSiteId = 1;
};
