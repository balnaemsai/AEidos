#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Core/Types/WorkTypes.h"
#include "WorkDefinitionRow.generated.h"

USTRUCT(BlueprintType)
struct FWorkDefinitionRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName WorkId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EWorkCategory WorkCategory = EWorkCategory::Craft;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float TotalWork = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float BaseWorkRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 MaxWorkers = 1;

	/** True면 요청 시 대상 Page를 지정해야 하며, 해당 Page만 이 Job을 수행한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bPageSpecificJob = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName SiteTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FWorkCost> Costs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FWorkReward> Rewards;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName PrimarySkillId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float XPPerSecond = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float XPFactor = 1.f;
};
