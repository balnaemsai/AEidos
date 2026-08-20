#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "DungeonAttributeDefinitionRow.generated.h"

/** CSV-authored rule for a thematic component that may be rolled for a dungeon. */
USTRUCT(BlueprintType)
struct FDungeonAttributeDefinitionRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName AttributeId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	/** Item created at the core for this component. It must exist in DT_Item. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName CoreShardItemId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.0"))
	float MinimumDifficulty = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.0"))
	float MaximumDifficulty = 999999.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.0"))
	float SelectionWeight = 1.f;

	/** Difficulty contributed by one point of this attribute's strength. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.01"))
	float DifficultyWeight = 1.f;

	/** Inclusive strength range used when a portal rolls this attribute. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1"))
	int32 MinimumStrength = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1"))
	int32 MaximumStrength = 1;

	/** One stack of this item is created for each selected component by default. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1"))
	int32 CoreShardQuantity = 1;
};
