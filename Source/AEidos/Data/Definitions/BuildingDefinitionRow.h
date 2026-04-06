#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "BuildingDefinitionRow.generated.h"

class AActor;

UENUM(BlueprintType)
enum class EBuildingCategory : uint8
{
	Production,
	Military,
	Research,
	Defense,
	Special,
	Structure
};

USTRUCT(BlueprintType)
struct FBuildingDefinitionRow : public FTableRowBase
{
	GENERATED_BODY()

	// RowName == BuildingId 권장
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName BuildingId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EBuildingCategory Category = EBuildingCategory::Production;

	// 월드에 배치될 실제 완공 건물 Actor
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftClassPtr<AActor> BuildingActorClass;

	// (선택) 공사장 표시용 Actor
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftClassPtr<AActor> ConstructionSiteActorClass;

	// 이 건물을 짓기 위해 WS_Work에 넣을 WorkId
	// 예: Construct_Farm
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName BuildWorkId;

	// 간단한 2D footprint (cm 기준)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector2D Footprint = FVector2D(200.f, 200.f);

	// 배치 높이 오프셋
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float ZOffset = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> ThumbnailIcon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FName> RequiredResearchIds;
};