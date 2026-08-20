#include "Entities/Items/InventoryComponent.h"

#include "Data/Definitions/ItemDefinitionRow.h"
#include "Data/GIS_DataRegistry.h"
#include "Engine/GameInstance.h"

namespace
{
	bool HasSameDungeonAttributes(const FItemStack& A, const FItemStack& B)
	{
		if (A.DungeonAttributes.Num() != B.DungeonAttributes.Num()) return false;
		for (int32 Index = 0; Index < A.DungeonAttributes.Num(); ++Index)
		{
			const FDungeonAttributeWeight& Left = A.DungeonAttributes[Index];
			const FDungeonAttributeWeight& Right = B.DungeonAttributes[Index];
			if (Left.AttributeId != Right.AttributeId || Left.Strength != Right.Strength || !FMath::IsNearlyEqual(Left.Weight, Right.Weight)) return false;
		}
		return true;
	}
}

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

const FItemDefinitionRow* UInventoryComponent::FindDefinition(FName ItemId) const
{
	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	UGIS_DataRegistry* Registry = GameInstance ? GameInstance->GetSubsystem<UGIS_DataRegistry>() : nullptr;
	return Registry && Registry->EnsureReadySync() ? Registry->GetItemDef(ItemId) : nullptr;
}

float UInventoryComponent::GetCurrentWeight() const
{
	float Result = 0.f;
	for (const FItemStack& Stack : Stacks)
	{
		if (const FItemDefinitionRow* Def = FindDefinition(Stack.ItemId))
		{
			Result += Def->UnitWeight * Stack.Quantity;
		}
	}
	return Result;
}

float UInventoryComponent::GetCurrentVolume() const
{
	float Result = 0.f;
	for (const FItemStack& Stack : Stacks)
	{
		if (const FItemDefinitionRow* Def = FindDefinition(Stack.ItemId))
		{
			Result += Def->UnitVolume * Stack.Quantity;
		}
	}
	return Result;
}

void UInventoryComponent::SetCapacity(float InMaxWeight, float InMaxVolume)
{
	MaxWeight = FMath::Max(0.f, InMaxWeight);
	MaxVolume = FMath::Max(0.f, InMaxVolume);
	BroadcastChanged();
}

int32 UInventoryComponent::TryAddItem(FName ItemId, int32 RequestedQuantity, float TotalQuality)
{
	const FItemDefinitionRow* Def = FindDefinition(ItemId);
	if (!Def || RequestedQuantity <= 0)
	{
		return 0;
	}

	// Page inventories deliberately accept overflow. Carrying too much is penalized by Page movement,
	// while settlement storage remains capacity-limited in UWS_ItemStorage.
	int32 Remaining = RequestedQuantity;
	int32 Added = 0;

	for (FItemStack& Stack : Stacks)
	{
		if (Remaining <= 0 || Stack.ItemId != ItemId || Stack.Quantity >= Def->StackLimit)
		{
			continue;
		}

		const int32 ToAdd = FMath::Min(Remaining, Def->StackLimit - Stack.Quantity);
		Stack.TotalQuality += RequestedQuantity > 0 ? TotalQuality * (static_cast<float>(ToAdd) / RequestedQuantity) : 0.f;
		Stack.Quantity += ToAdd;
		Remaining -= ToAdd;
		Added += ToAdd;
	}

	while (Remaining > 0)
	{
		const int32 ToAdd = FMath::Min(Remaining, Def->StackLimit);
		FItemStack NewStack;
		NewStack.ItemId = ItemId;
		NewStack.Quantity = ToAdd;
		NewStack.TotalQuality = RequestedQuantity > 0 ? TotalQuality * (static_cast<float>(ToAdd) / RequestedQuantity) : 0.f;
		Stacks.Add(NewStack);
		Remaining -= ToAdd;
		Added += ToAdd;
	}

	if (Added > 0)
	{
		BroadcastChanged();
	}
	return Added;
}

int32 UInventoryComponent::TryAddItemStack(const FItemStack& ItemStack)
{
	if (!ItemStack.IsValid()) return 0;
	if (ItemStack.DungeonAttributes.IsEmpty()) return TryAddItem(ItemStack.ItemId, ItemStack.Quantity, ItemStack.TotalQuality);

	const FItemDefinitionRow* Def = FindDefinition(ItemStack.ItemId);
	if (!Def || ItemStack.Quantity > Def->StackLimit) return 0;
	Stacks.Add(ItemStack);
	BroadcastChanged();
	return ItemStack.Quantity;
}

int32 UInventoryComponent::TryRemoveItem(FName ItemId, int32 RequestedQuantity, float& OutRemovedQuality)
{
	OutRemovedQuality = 0.f;
	int32 Remaining = FMath::Max(0, RequestedQuantity);
	int32 Removed = 0;
	for (int32 Index = Stacks.Num() - 1; Index >= 0 && Remaining > 0; --Index)
	{
		FItemStack& Stack = Stacks[Index];
		if (Stack.ItemId != ItemId)
		{
			continue;
		}

		const int32 ToRemove = FMath::Min(Remaining, Stack.Quantity);
		const float QualityToRemove = Stack.Quantity > 0 ? Stack.TotalQuality * (static_cast<float>(ToRemove) / Stack.Quantity) : 0.f;
		Stack.Quantity -= ToRemove;
		Stack.TotalQuality -= QualityToRemove;
		OutRemovedQuality += QualityToRemove;
		Remaining -= ToRemove;
		Removed += ToRemove;
		if (Stack.Quantity <= 0)
		{
			Stacks.RemoveAt(Index);
		}
	}

	if (Removed > 0)
	{
		BroadcastChanged();
	}
	return Removed;
}

bool UInventoryComponent::TryRemoveItemStack(const FItemStack& ItemStack)
{
	for (int32 Index = 0; Index < Stacks.Num(); ++Index)
	{
		if (Stacks[Index].ItemId == ItemStack.ItemId && Stacks[Index].Quantity == ItemStack.Quantity
			&& HasSameDungeonAttributes(Stacks[Index], ItemStack))
		{
			Stacks.RemoveAt(Index);
			BroadcastChanged();
			return true;
		}
	}
	return false;
}

void UInventoryComponent::SetStacks(const TArray<FItemStack>& InStacks)
{
	Stacks = InStacks;
	Stacks.RemoveAll([](const FItemStack& Stack) { return !Stack.IsValid(); });
	BroadcastChanged();
}

void UInventoryComponent::BroadcastChanged()
{
	OnInventoryChanged.Broadcast();
}
