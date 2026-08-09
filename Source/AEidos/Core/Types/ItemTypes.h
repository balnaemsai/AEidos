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

UENUM(BlueprintType)
enum class EPageEquipmentSlot : uint8
{
	LeftHand,
	RightHand,
	Head,
	UpperBody,
	LowerBody,
	Feet
};

/** Commands that an item may expose from an inventory context menu. */
UENUM(BlueprintType)
enum class EInventoryItemActionType : uint8
{
	MoveToOtherInventory,
	Drop,
	Use,
	Place,
	Equip
};

/** Built-in effects that an inventory item's Use command can apply without a Blueprint. */
UENUM(BlueprintType)
enum class EItemUseEffectType : uint8
{
	None,
	RestoreHealth
};

USTRUCT(BlueprintType)
struct FPageEquipmentSlotState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPageEquipmentSlot Slot = EPageEquipmentSlot::RightHand;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemId = NAME_None;
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
