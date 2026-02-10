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

	FIntPoint GetCoord() const { return Coord; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* Mesh;

	// 좌표(그리드)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FIntPoint Coord = FIntPoint::ZeroValue;

	// 런타임 재질(파라미터 조절용)
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* MID = nullptr;

	// 기본 Plane Mesh / Material은 BP에서 바꿀 수 있게 열어둠
	UPROPERTY(EditDefaultsOnly, Category="Territory")
	UStaticMesh* PlaneMesh = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Territory")
	UMaterialInterface* TerritoryMaterial = nullptr;

};
