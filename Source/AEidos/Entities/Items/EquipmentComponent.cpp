#include "Entities/Items/EquipmentComponent.h"

#include "Data/Definitions/ItemDefinitionRow.h"
#include "Data/GIS_DataRegistry.h"
#include "Engine/GameInstance.h"
#include "Entities/Items/InventoryComponent.h"
#include "Entities/Page/PageCharacter.h"

UEquipmentComponent::UEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	EnsureSlots();
}

void UEquipmentComponent::EnsureSlots()
{
	for (int32 Index = 0; Index <= static_cast<int32>(EPageEquipmentSlot::Feet); ++Index)
	{
		const EPageEquipmentSlot Slot = static_cast<EPageEquipmentSlot>(Index);
		if (!FindSlot(Slot))
		{
			FPageEquipmentSlotState& State = EquippedSlots.AddDefaulted_GetRef();
			State.Slot = Slot;
		}
	}
}

const FItemDefinitionRow* UEquipmentComponent::FindItemDefinition(FName ItemId) const
{
	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	UGIS_DataRegistry* Registry = GameInstance ? GameInstance->GetSubsystem<UGIS_DataRegistry>() : nullptr;
	return Registry && Registry->EnsureReadySync() ? Registry->GetItemDef(ItemId) : nullptr;
}

UInventoryComponent* UEquipmentComponent::GetOwnerInventory() const
{
	const APageCharacter* Page = Cast<APageCharacter>(GetOwner());
	return Page ? Page->GetInventory() : nullptr;
}

FPageEquipmentSlotState* UEquipmentComponent::FindSlot(EPageEquipmentSlot Slot)
{
	return EquippedSlots.FindByPredicate([Slot](const FPageEquipmentSlotState& State) { return State.Slot == Slot; });
}

const FPageEquipmentSlotState* UEquipmentComponent::FindSlot(EPageEquipmentSlot Slot) const
{
	return EquippedSlots.FindByPredicate([Slot](const FPageEquipmentSlotState& State) { return State.Slot == Slot; });
}

void UEquipmentComponent::SetEquippedSlots(const TArray<FPageEquipmentSlotState>& InSlots)
{
	EquippedSlots = InSlots;
	EnsureSlots();
	OnEquipmentChanged.Broadcast();
}

FName UEquipmentComponent::GetEquippedItem(EPageEquipmentSlot Slot) const
{
	const FPageEquipmentSlotState* State = FindSlot(Slot);
	return State ? State->ItemId : NAME_None;
}

FName UEquipmentComponent::GetActiveToolItem() const
{
	const FName RightHand = GetEquippedItem(EPageEquipmentSlot::RightHand);
	return !RightHand.IsNone() ? RightHand : GetEquippedItem(EPageEquipmentSlot::LeftHand);
}

bool UEquipmentComponent::HasActiveToolTag(FName ToolTag) const
{
	if (ToolTag.IsNone()) return true;
	const FName ActiveTool = GetActiveToolItem();
	if (ActiveTool.IsNone()) return ToolTag == TEXT("Tool.BareHand");
	const FItemDefinitionRow* Def = FindItemDefinition(ActiveTool);
	return Def && Def->ToolInteractionTags.Contains(ToolTag);
}

bool UEquipmentComponent::EquipFromInventory(FName ItemId, EPageEquipmentSlot Slot)
{
	UInventoryComponent* Inventory = GetOwnerInventory();
	const FItemDefinitionRow* Def = FindItemDefinition(ItemId);
	if (!Inventory || !Def || Def->ItemType != EItemType::Equipment || !Def->CompatibleEquipmentSlots.Contains(Slot)) return false;
	float RemovedQuality = 0.f;
	if (Inventory->TryRemoveItem(ItemId, 1, RemovedQuality) != 1) return false;
	FPageEquipmentSlotState* TargetSlot = FindSlot(Slot);
	if (!TargetSlot)
	{
		Inventory->TryAddItem(ItemId, 1, RemovedQuality);
		return false;
	}
	if (!TargetSlot->ItemId.IsNone()) Inventory->TryAddItem(TargetSlot->ItemId, 1);
	TargetSlot->ItemId = ItemId;
	OnEquipmentChanged.Broadcast();
	return true;
}

bool UEquipmentComponent::UnequipToInventory(EPageEquipmentSlot Slot)
{
	UInventoryComponent* Inventory = GetOwnerInventory();
	FPageEquipmentSlotState* TargetSlot = FindSlot(Slot);
	if (!Inventory || !TargetSlot || TargetSlot->ItemId.IsNone()) return false;
	Inventory->TryAddItem(TargetSlot->ItemId, 1);
	TargetSlot->ItemId = NAME_None;
	OnEquipmentChanged.Broadcast();
	return true;
}
