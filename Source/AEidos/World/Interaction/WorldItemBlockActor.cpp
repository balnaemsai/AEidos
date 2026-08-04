#include "World/Interaction/WorldItemBlockActor.h"

#include "Components/StaticMeshComponent.h"
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
	PrimaryActorTick.bCanEverTick = false;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionObjectType(ECC_WorldDynamic);
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}

bool AWorldItemBlockActor::CanInteract(APageCharacter* InteractingPage) const
{
	return InteractingPage && !ItemId.IsNone() && RemainingQuantity > 0
		&& FVector::DistSquared(InteractingPage->GetActorLocation(), GetActorLocation()) <= FMath::Square(InteractionRangeCm);
}

bool AWorldItemBlockActor::HasRequiredTool(APageCharacter* InteractingPage) const
{
	return RequiredHarvestToolTag.IsNone() || (InteractingPage && InteractingPage->GetEquipment()
		&& InteractingPage->GetEquipment()->HasActiveToolTag(RequiredHarvestToolTag));
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
