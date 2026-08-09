#include "World/Interaction/WorldItemBlockActor.h"

#include "Entities/Items/EquipmentComponent.h"
#include "Entities/Items/InventoryComponent.h"
#include "Entities/Page/PageCharacter.h"

namespace
{
	const FName PickupInteractionId(TEXT("Pickup"));
	const FName HarvestInteractionId(TEXT("Harvest"));
}

AWorldItemBlockActor::AWorldItemBlockActor()
{
}

void AWorldItemBlockActor::GetBlockInteractionDefinitions(TArray<FWorldBlockInteractionDefinition>& OutDefinitions) const
{
	OutDefinitions.Reset();
	if (bCanHarvest)
	{
		FWorldBlockInteractionDefinition& Harvest = OutDefinitions.AddDefaulted_GetRef();
		Harvest.InteractionId = HarvestInteractionId;
		Harvest.DisplayName = FText::FromString(TEXT("Harvest"));
		Harvest.Description = FText::FromString(TEXT("Harvest this block with the equipped tool."));
		Harvest.RequiredToolTag = RequiredHarvestToolTag;
		Harvest.ResultItemId = ItemId;
		Harvest.ResultQuantity = QuantityPerHarvest;
		Harvest.bIsDefault = true;
	}
	if (bCanPickUp)
	{
		FWorldBlockInteractionDefinition& Pickup = OutDefinitions.AddDefaulted_GetRef();
		Pickup.InteractionId = PickupInteractionId;
		Pickup.DisplayName = FText::FromString(TEXT("Pick Up"));
		Pickup.Description = FText::FromString(TEXT("Put this block's contents into the Page inventory."));
		Pickup.ResultItemId = ItemId;
		Pickup.ResultQuantity = RemainingQuantity;
		Pickup.bIsDefault = !bCanHarvest;
	}
}

void AWorldItemBlockActor::ApplyDungeonBlockPresetData(FName InBlockId, int32 InRemainingIntegrity,
	const TArray<FWorldBlockInteractionDefinition>& InInteractions)
{
	ItemId = InBlockId;
	RemainingQuantity = FMath::Max(1, InRemainingIntegrity);
	bCanHarvest = false;
	bCanPickUp = false;
	RequiredHarvestToolTag = NAME_None;
	QuantityPerHarvest = 1;

	for (const FWorldBlockInteractionDefinition& Interaction : InInteractions)
	{
		if (!Interaction.ResultItemId.IsNone()) ItemId = Interaction.ResultItemId;
		if (Interaction.InteractionId == HarvestInteractionId)
		{
			bCanHarvest = true;
			RequiredHarvestToolTag = Interaction.RequiredToolTag;
			QuantityPerHarvest = FMath::Max(1, Interaction.ResultQuantity);
		}
		else if (Interaction.InteractionId == PickupInteractionId)
		{
			bCanPickUp = true;
		}
	}
}

void AWorldItemBlockActor::ApplyDungeonPresetData(FName InItemId, int32 InRemainingQuantity,
	int32 InQuantityPerHarvest, bool bInCanPickUp, bool bInCanHarvest, FName InRequiredHarvestToolTag)
{
	ItemId = InItemId;
	RemainingQuantity = FMath::Max(1, InRemainingQuantity);
	QuantityPerHarvest = FMath::Max(1, InQuantityPerHarvest);
	bCanPickUp = bInCanPickUp;
	bCanHarvest = bInCanHarvest;
	RequiredHarvestToolTag = InRequiredHarvestToolTag;
}

void AWorldItemBlockActor::InitializeWorldItem(FName InItemId, int32 InQuantity)
{
	ItemId = InItemId;
	RemainingQuantity = FMath::Max(1, InQuantity);
	QuantityPerHarvest = 1;
	bCanPickUp = true;
	bCanHarvest = false;
	RequiredHarvestToolTag = NAME_None;
}

bool AWorldItemBlockActor::CanInteract(APageCharacter* InteractingPage) const
{
	return InteractingPage && !ItemId.IsNone() && RemainingQuantity > 0
		&& FVector::DistSquared(InteractingPage->GetActorLocation(), GetActorLocation()) <= FMath::Square(InteractionRangeCm);
}

bool AWorldItemBlockActor::HasRequiredTool(APageCharacter* InteractingPage) const
{
	return RequiredHarvestToolTag.IsNone() || (InteractingPage && InteractingPage->GetEquipment()
		&& InteractingPage->GetEquipment()->CanUseToolForInteraction(RequiredHarvestToolTag));
}

void AWorldItemBlockActor::GetAvailableWorldInteractions_Implementation(APageCharacter* InteractingPage, TArray<FWorldInteractionOption>& OutOptions)
{
	OutOptions.Reset();
	if (!CanInteract(InteractingPage)) return;
	if (bCanHarvest)
	{
		FWorldInteractionOption& Harvest = OutOptions.AddDefaulted_GetRef();
		Harvest.InteractionId = HarvestInteractionId;
		Harvest.DisplayName = FText::FromString(TEXT("Harvest"));
		Harvest.Description = RequiredHarvestToolTag.IsNone() ? FText::FromString(TEXT("Collect material with the active hand tool."))
			: FText::Format(FText::FromString(TEXT("Requires {0}")), FText::FromName(RequiredHarvestToolTag));
		Harvest.RequiredToolTag = RequiredHarvestToolTag;
		Harvest.bIsDefault = true;
	}
	if (bCanPickUp)
	{
		FWorldInteractionOption& Pickup = OutOptions.AddDefaulted_GetRef();
		Pickup.InteractionId = PickupInteractionId;
		Pickup.DisplayName = FText::FromString(TEXT("Pick Up"));
		Pickup.Description = FText::FromString(TEXT("Put the object into the Page inventory."));
		Pickup.bIsDefault = !bCanHarvest;
	}
}

bool AWorldItemBlockActor::TransferToInventory(APageCharacter* InteractingPage, int32 Amount)
{
	UInventoryComponent* Inventory = InteractingPage ? InteractingPage->GetInventory() : nullptr;
	if (!Inventory || Amount <= 0) return false;
	const int32 Added = Inventory->TryAddItem(ItemId, FMath::Min(Amount, RemainingQuantity));
	if (Added <= 0) return false;
	RemainingQuantity -= Added;
	if (RemainingQuantity <= 0) Destroy();
	return true;
}

bool AWorldItemBlockActor::ExecuteWorldInteraction_Implementation(APageCharacter* InteractingPage, FName InteractionId)
{
	if (!CanInteract(InteractingPage)) return false;
	if (InteractionId == HarvestInteractionId && bCanHarvest)
	{
		return HasRequiredTool(InteractingPage) && TransferToInventory(InteractingPage, QuantityPerHarvest);
	}
	if (InteractionId == PickupInteractionId && bCanPickUp) return TransferToInventory(InteractingPage, RemainingQuantity);
	return false;
}
