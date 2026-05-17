// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TerritoryChunkActor.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

UCLASS()
class AEIDOS_API ATerritoryChunkActor : public AActor
{
	GENERATED_BODY()
	
public:
	ATerritoryChunkActor();

	void InitChunk(const FIntPoint& InCoord, float InChunkSizeCm);
	void SetPreviewMode(bool bInPreviewMode);
	void SetPreviewValid(bool bInPreviewValid);

	FIntPoint GetCoord() const { return Coord; }

#if WITH_EDITOR
	virtual void OnConstruction(const FTransform& Transform) override;
#endif

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* Mesh;

	// 좌표(그리드)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FIntPoint Coord = FIntPoint::ZeroValue;

	// 런타임 재질(파라미터 조절용)
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* MID = nullptr;

	UPROPERTY(Transient)
	UMaterialInstanceDynamic* PreviewOverlayMID = nullptr;

	UPROPERTY(Transient)
	bool bPreviewMode = false;

	UPROPERTY(Transient)
	bool bPreviewValid = true;

	// 기본 Plane Mesh / Material은 BP에서 바꿀 수 있게 열어둠
	UPROPERTY(EditDefaultsOnly, Category="Territory")
	UStaticMesh* PlaneMesh = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Territory")
	UMaterialInterface* TerritoryMaterial = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Territory|Preview")
	UMaterialInterface* PreviewOverlayMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Territory|Authoring")
	bool bUseEditorAuthoringCoord = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Territory|Authoring", meta=(EditCondition="bUseEditorAuthoringCoord"))
	FIntPoint EditorCoord = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Territory|Authoring", meta=(ClampMin="100.0"))
	float EditorChunkSizeCm = 1000.f;

	void RefreshPreviewVisual();
};
