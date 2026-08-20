#pragma once

#include "CoreMinimal.h"
#include "DungeonTypes.generated.h"

/** A deterministic composition component captured when a portal is created. */
USTRUCT(BlueprintType)
struct FDungeonAttributeWeight
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName AttributeId = NAME_None;

	/** Normalized map-composition share of this attribute within the dungeon. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Weight = 0.f;

	/** Integer attribute strength retained by the portal shard and absorbed at an EP Altar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0"))
	int32 Strength = 0;
};
