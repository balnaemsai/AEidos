#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ConstructionSiteActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UBoxComponent;
class UMaterialInstanceDynamic;

UCLASS()
class AEIDOS_API AConstructionSiteActor : public AActor
{
	GENERATED_BODY()

public:
	AConstructionSiteActor();

	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintCallable, Category="Construction")
	void InitializeConstructionSite(int32 InSiteId, FName InBuildingId, int32 InWorkRequestId);

	UFUNCTION(BlueprintCallable, Category="Construction")
	void SetFootprint(const FVector2D& InFootprint);

	UFUNCTION(BlueprintCallable, Category="Construction")
	void SetPreviewValid(bool bIsValidPlacement);

	UFUNCTION(BlueprintCallable, Category="Construction")
	void SetConstructionProgress(float InNormalizedProgress);

	UFUNCTION(BlueprintPure, Category="Construction")
	int32 GetSiteId() const { return SiteId; }

	UFUNCTION(BlueprintPure, Category="Construction")
	FName GetBuildingId() const { return BuildingId; }

	UFUNCTION(BlueprintPure, Category="Construction")
	int32 GetWorkRequestId() const { return WorkRequestId; }

	UFUNCTION(BlueprintPure, Category="Construction")
	float GetConstructionProgress() const { return ConstructionProgress; }

	UFUNCTION(BlueprintPure, Category="Construction")
	UStaticMeshComponent* GetMeshComponent() const { return SiteMesh; }

	UFUNCTION(BlueprintPure, Category="Construction")
	UBoxComponent* GetFootprintCollision() const { return FootprintCollision; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Construction|Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Construction|Components")
	TObjectPtr<UStaticMeshComponent> SiteMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Construction|Components")
	TObjectPtr<UBoxComponent> FootprintCollision;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Construction")
	int32 SiteId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Construction")
	FName BuildingId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Construction")
	int32 WorkRequestId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Construction")
	FVector2D Footprint = FVector2D(200.f, 200.f);

	// 0~1
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Construction")
	float ConstructionProgress = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Construction")
	bool bPreviewValid = true;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PreviewMID;

protected:
	void RefreshCollisionExtent();
	void RefreshPreviewVisual();
};