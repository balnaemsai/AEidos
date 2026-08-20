#pragma once

#include "CoreMinimal.h"
#include "Core/Types/ItemTypes.h"
#include "Engine/DataTable.h"
#include "ItemDefinitionRow.generated.h"

class AActor;
class AWorldBlockActor;
class UTexture2D;

USTRUCT(BlueprintType)
struct FItemDefinitionRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName ItemId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(MultiLine=true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EItemType ItemType = EItemType::Material;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1"))
	int32 StackLimit = 100;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.0"))
	float UnitWeight = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.0"))
	float UnitVolume = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftClassPtr<AActor> WorldPickupClass;

	// When assigned, this inventory item can be consumed to place the specified world block.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="World Block")
	TSoftClassPtr<AWorldBlockActor> PlacedBlockClass;

	// CSV format: (Use,Place). Move and Drop are contextual built-ins, while this list enables item-specific commands.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inventory")
	TArray<EInventoryItemActionType> InventoryActions;

	/** Built-in result of the Use command. None delegates to the UI extension hook. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Use")
	EItemUseEffectType UseEffect = EItemUseEffectType::None;

	/** Magnitude interpreted by UseEffect; RestoreHealth uses health points. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Use", meta=(ClampMin="0.0"))
	float UseEffectMagnitude = 0.f;

	/** A successful built-in Use removes one item from the selected Page inventory. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Use")
	bool bConsumeOnUse = true;

	// Equipment items may occupy one of these Page slots. Empty means the item cannot be equipped.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment")
	TArray<EPageEquipmentSlot> CompatibleEquipmentSlots;

	// Tags understood by world interactions, e.g. Tool.Pickaxe or Tool.Axe.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment")
	TArray<FName> ToolInteractionTags;

	// Generic equipment tags used by combat skills, e.g. Weapon.Melee or Weapon.Throwable.
	// ToolInteractionTags remain supported so existing gathering tools need no migration.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment")
	TArray<FName> EquipmentTags;

	// Non-None items turn into this warehouse resource when a Page returns.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName SettlementResourceId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bConvertOnReturn = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bTracksQuality = false;

	/**
	 * Prepared meal units supplied by one of this item to the settlement-wide
	 * meal service. Zero means this item is not a prepared meal.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sustenance", meta=(ClampMin="0.0"))
	float SettlementMealUnits = 0.f;

	/** Base quality used when a legacy stack has no explicit quality value. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sustenance", meta=(ClampMin="0.0"))
	float DefaultMealQuality = 0.f;
};
