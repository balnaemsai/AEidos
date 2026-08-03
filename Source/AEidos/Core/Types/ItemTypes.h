#pragma once

#include "CoreMinimal.h"
#include "ItemTypes.generated.h"

UENUM(BlueprintType)
enum class EItemType : uint8
{
	Material,
	Consumable,
	CoreShard,
	Blueprint,
	Equipment,
	Quest
};

/** A stack is intentionally value-based so it can be owned by Pages, storage, or world pickups. */
USTRUCT(BlueprintType)
struct FItemStack
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0"))
	int32 Quantity = 0;

	// Sum rather than average so differently produced meals can merge safely.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TotalQuality = 0.f;

	bool IsValid() const { return !ItemId.IsNone() && Quantity > 0; }
};
