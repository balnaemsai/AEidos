#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ScenarioDefinitionRow.generated.h"

/**
 * A data-only objective set. Empty requirement groups are ignored; a scenario
 * only completes once every populated requirement has been met.
 */
USTRUCT(BlueprintType)
struct FScenarioDefinitionRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName ScenarioId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(MultiLine="true"))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FName> RequiredResearchIds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FName> RequiredBuildingIds;

	/** Number of dungeon cores the settlement must destroy during this save. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0"))
	int32 RequiredDungeonCoreDestructions = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0"))
	int32 RequiredPageCount = 0;
};
