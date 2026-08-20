#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "DungeonPresetDefinitionRow.generated.h"

class UDungeonSettlementPreset;

/** CSV-authored candidate used to select the settlement layout for a dungeon. */
USTRUCT(BlueprintType)
struct FDungeonPresetDefinitionRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName PresetId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UDungeonSettlementPreset> PresetAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.0"))
	float MinimumDifficulty = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.0"))
	float MaximumDifficulty = 999999.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.0"))
	float SelectionWeight = 1.f;
};
