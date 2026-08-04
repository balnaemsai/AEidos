#pragma once

#include "CoreMinimal.h"
#include "Core/Types/ItemTypes.h"
#include "Engine/DataTable.h"
#include "ItemDefinitionRow.generated.h"

class AActor;
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

	// Equipment items may occupy one of these Page slots. Empty means the item cannot be equipped.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment")
	TArray<EPageEquipmentSlot> CompatibleEquipmentSlots;

	// Tags understood by world interactions, e.g. Tool.Pickaxe or Tool.Axe.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment")
	TArray<FName> ToolInteractionTags;

	// Non-None items turn into this warehouse resource when a Page returns.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName SettlementResourceId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bConvertOnReturn = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bTracksQuality = false;
};
