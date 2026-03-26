#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BuildingActorBase.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UBoxComponent;

UCLASS()
class AEIDOS_API ABuildingActorBase : public AActor
{
	GENERATED_BODY()

public:
	ABuildingActorBase();

	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintCallable, Category="Building")
	void InitializeBuilding(FName InBuildingId, int32 InUpgradeLevel = 1);

	UFUNCTION(BlueprintPure, Category="Building")
	FName GetBuildingId() const { return BuildingId; }

	UFUNCTION(BlueprintPure, Category="Building")
	int32 GetUpgradeLevel() const { return UpgradeLevel; }

	UFUNCTION(BlueprintCallable, Category="Building")
	void SetFootprint(const FVector2D& InFootprint);

	UFUNCTION(BlueprintPure, Category="Building")
	FVector2D GetFootprint() const { return Footprint; }

	UFUNCTION(BlueprintCallable, Category="Building")
	void SetBuildingActive(bool bInActive);

	UFUNCTION(BlueprintPure, Category="Building")
	bool IsBuildingActive() const { return bBuildingActive; }

	UFUNCTION(BlueprintPure, Category="Building")
	UStaticMeshComponent* GetMeshComponent() const { return BuildingMesh; }

	UFUNCTION(BlueprintPure, Category="Building")
	UBoxComponent* GetFootprintCollision() const { return FootprintCollision; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Building|Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Building|Components")
	TObjectPtr<UStaticMeshComponent> BuildingMesh;

	// 건물 점유 영역/선택용
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Building|Components")
	TObjectPtr<UBoxComponent> FootprintCollision;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Building")
	FName BuildingId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Building")
	int32 UpgradeLevel = 1;

	// cm 기준 footprint
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Building")
	FVector2D Footprint = FVector2D(200.f, 200.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Building")
	bool bBuildingActive = true;

protected:
	void RefreshCollisionExtent();
};