#include "World/Settlement/WS_ItemStorage.h"

#include "Data/Definitions/BuildingDefinitionRow.h"
#include "Data/Definitions/ItemDefinitionRow.h"
#include "Data/Definitions/ResourceDefinitionRow.h"
#include "Data/GIS_DataRegistry.h"
#include "Engine/GameInstance.h"
#include "Entities/Items/InventoryComponent.h"
#include "Entities/Page/PageCharacter.h"
#include "World/Settlement/WS_Building.h"
#include "World/Settlement/WS_Economy.h"

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

	FString EncodeDungeonAttributes(const TArray<FDungeonAttributeWeight>& Attributes)
	{
		TArray<FString> Parts;
		for (const FDungeonAttributeWeight& Attribute : Attributes)
		{
			if (!Attribute.AttributeId.IsNone() && Attribute.Strength > 0)
			{
				Parts.Add(FString::Printf(TEXT("%s:%.4f:%d"), *Attribute.AttributeId.ToString(), Attribute.Weight, Attribute.Strength));
			}
		}
		return FString::Join(Parts, TEXT("|"));
	}

	void DecodeDungeonAttributes(const FString& Encoded, TArray<FDungeonAttributeWeight>& OutAttributes)
	{
		OutAttributes.Reset();
		TArray<FString> Parts;
		Encoded.ParseIntoArray(Parts, TEXT("|"), true);
		for (const FString& Part : Parts)
		{
			TArray<FString> Fields;
			Part.ParseIntoArray(Fields, TEXT(":"), false);
			if (Fields.Num() != 3 || Fields[0].IsEmpty()) continue;
			const int32 Strength = FCString::Atoi(*Fields[2]);
			if (Strength > 0) OutAttributes.Add({ FName(*Fields[0]), FCString::Atof(*Fields[1]), Strength });
		}
	}
}

void UWS_ItemStorage::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UWS_Building>();
}

FName UWS_ItemStorage::SnapshotKey()
{
	return TEXT("ItemStorage.StoredItems");
}

const FItemDefinitionRow* UWS_ItemStorage::FindItemDefinition(FName ItemId) const
{
	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UGIS_DataRegistry* Registry = GameInstance ? GameInstance->GetSubsystem<UGIS_DataRegistry>() : nullptr;
	return Registry && Registry->EnsureReadySync() ? Registry->GetItemDef(ItemId) : nullptr;
}

const FResourceDefinitionRow* UWS_ItemStorage::FindResourceDefinition(FName ResourceId) const
{
	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UGIS_DataRegistry* Registry = GameInstance ? GameInstance->GetSubsystem<UGIS_DataRegistry>() : nullptr;
	return Registry && Registry->EnsureReadySync() ? Registry->GetResourceDef(ResourceId) : nullptr;
}

float UWS_ItemStorage::GetTotalWeightCapacity() const
{
	float Result = BaseWeightCapacity;
	UWS_Building* BuildingSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UWS_Building>() : nullptr;
	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UGIS_DataRegistry* Registry = GameInstance ? GameInstance->GetSubsystem<UGIS_DataRegistry>() : nullptr;
	if (!BuildingSubsystem || !Registry || !Registry->EnsureReadySync())
	{
		return Result;
	}

	TArray<FName> CompletedBuildingIds;
	BuildingSubsystem->GetCompletedBuildingIds(CompletedBuildingIds);
	for (const FName BuildingId : CompletedBuildingIds)
	{
		if (const FBuildingDefinitionRow* Def = Registry->GetBuildingDef(BuildingId))
		{
			Result += Def->StorageWeightCapacity;
		}
	}
	return Result;
}

float UWS_ItemStorage::GetTotalVolumeCapacity() const
{
	float Result = BaseVolumeCapacity;
	UWS_Building* BuildingSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UWS_Building>() : nullptr;
	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UGIS_DataRegistry* Registry = GameInstance ? GameInstance->GetSubsystem<UGIS_DataRegistry>() : nullptr;
	if (!BuildingSubsystem || !Registry || !Registry->EnsureReadySync())
	{
		return Result;
	}

	TArray<FName> CompletedBuildingIds;
	BuildingSubsystem->GetCompletedBuildingIds(CompletedBuildingIds);
	for (const FName BuildingId : CompletedBuildingIds)
	{
		if (const FBuildingDefinitionRow* Def = Registry->GetBuildingDef(BuildingId))
		{
			Result += Def->StorageVolumeCapacity;
		}
	}
	return Result;
}

float UWS_ItemStorage::GetCurrentWeight() const
{
	float Result = 0.f;
	UWS_Economy* Economy = GetWorld() ? GetWorld()->GetSubsystem<UWS_Economy>() : nullptr;
	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UGIS_DataRegistry* Registry = GameInstance ? GameInstance->GetSubsystem<UGIS_DataRegistry>() : nullptr;
	if (Economy && Registry && Registry->EnsureReadySync())
	{
		for (const FName ResourceId : Registry->GetAllResourceIds())
		{
			if (const FResourceDefinitionRow* Def = Registry->GetResourceDef(ResourceId))
			{
				Result += Def->UnitWeight * Economy->GetAmount(ResourceId);
			}
		}
	}

	for (const FItemStack& Stack : StoredItems)
	{
		if (const FItemDefinitionRow* Def = FindItemDefinition(Stack.ItemId))
		{
			Result += Def->UnitWeight * Stack.Quantity;
		}
	}
	return Result;
}

float UWS_ItemStorage::GetCurrentVolume() const
{
	float Result = 0.f;
	UWS_Economy* Economy = GetWorld() ? GetWorld()->GetSubsystem<UWS_Economy>() : nullptr;
	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UGIS_DataRegistry* Registry = GameInstance ? GameInstance->GetSubsystem<UGIS_DataRegistry>() : nullptr;
	if (Economy && Registry && Registry->EnsureReadySync())
	{
		for (const FName ResourceId : Registry->GetAllResourceIds())
		{
			if (const FResourceDefinitionRow* Def = Registry->GetResourceDef(ResourceId))
			{
				Result += Def->UnitVolume * Economy->GetAmount(ResourceId);
			}
		}
	}

	for (const FItemStack& Stack : StoredItems)
	{
		if (const FItemDefinitionRow* Def = FindItemDefinition(Stack.ItemId))
		{
			Result += Def->UnitVolume * Stack.Quantity;
		}
	}
	return Result;
}

int32 UWS_ItemStorage::GetStoredItemAmount(FName ItemId) const
{
	int32 Total = 0;
	for (const FItemStack& Stack : StoredItems)
	{
		if (Stack.ItemId == ItemId)
		{
			Total += Stack.Quantity;
		}
	}
	return Total;
}

bool UWS_ItemStorage::CanStoreItemStacks(const TArray<FItemStack>& ItemStacks) const
{
	float RequiredWeight = 0.f;
	float RequiredVolume = 0.f;
	for (const FItemStack& Stack : ItemStacks)
	{
		if (Stack.ItemId.IsNone() || Stack.Quantity <= 0)
		{
			continue;
		}

		const FItemDefinitionRow* Def = FindItemDefinition(Stack.ItemId);
		if (!Def)
		{
			return false;
		}

		RequiredWeight += Def->UnitWeight * Stack.Quantity;
		RequiredVolume += Def->UnitVolume * Stack.Quantity;
	}

	return RequiredWeight <= FMath::Max(0.f, GetTotalWeightCapacity() - GetCurrentWeight())
		&& RequiredVolume <= FMath::Max(0.f, GetTotalVolumeCapacity() - GetCurrentVolume());
}

int32 UWS_ItemStorage::GetMaxResourceAmountThatFits(FName ResourceId, int32 RequestedAmount) const
{
	const FResourceDefinitionRow* Def = FindResourceDefinition(ResourceId);
	if (!Def || RequestedAmount <= 0)
	{
		return 0;
	}

	const float FreeWeight = FMath::Max(0.f, GetTotalWeightCapacity() - GetCurrentWeight());
	const float FreeVolume = FMath::Max(0.f, GetTotalVolumeCapacity() - GetCurrentVolume());
	const int32 ByWeight = Def->UnitWeight > 0.f ? FMath::FloorToInt(FreeWeight / Def->UnitWeight) : RequestedAmount;
	const int32 ByVolume = Def->UnitVolume > 0.f ? FMath::FloorToInt(FreeVolume / Def->UnitVolume) : RequestedAmount;
	return FMath::Max(0, FMath::Min3(RequestedAmount, ByWeight, ByVolume));
}

int32 UWS_ItemStorage::TryStoreItem(FName ItemId, int32 RequestedQuantity, float TotalQuality)
{
	const FItemDefinitionRow* Def = FindItemDefinition(ItemId);
	if (!Def || RequestedQuantity <= 0)
	{
		return 0;
	}

	const float FreeWeight = FMath::Max(0.f, GetTotalWeightCapacity() - GetCurrentWeight());
	const float FreeVolume = FMath::Max(0.f, GetTotalVolumeCapacity() - GetCurrentVolume());
	const int32 ByWeight = Def->UnitWeight > 0.f ? FMath::FloorToInt(FreeWeight / Def->UnitWeight) : RequestedQuantity;
	const int32 ByVolume = Def->UnitVolume > 0.f ? FMath::FloorToInt(FreeVolume / Def->UnitVolume) : RequestedQuantity;
	int32 Remaining = FMath::Min3(RequestedQuantity, ByWeight, ByVolume);
	int32 Added = 0;

	for (FItemStack& Stack : StoredItems)
	{
		if (Remaining <= 0 || Stack.ItemId != ItemId || Stack.Quantity >= Def->StackLimit)
		{
			continue;
		}
		const int32 ToAdd = FMath::Min(Remaining, Def->StackLimit - Stack.Quantity);
		Stack.TotalQuality += TotalQuality * (static_cast<float>(ToAdd) / RequestedQuantity);
		Stack.Quantity += ToAdd;
		Remaining -= ToAdd;
		Added += ToAdd;
	}

	while (Remaining > 0)
	{
		const int32 ToAdd = FMath::Min(Remaining, Def->StackLimit);
		StoredItems.Add({ItemId, ToAdd, TotalQuality * (static_cast<float>(ToAdd) / RequestedQuantity)});
		Remaining -= ToAdd;
		Added += ToAdd;
	}

	if (Added > 0)
	{
		BroadcastChanged();
	}
	return Added;
}

int32 UWS_ItemStorage::TryStoreItemStack(const FItemStack& ItemStack)
{
	if (!ItemStack.IsValid()) return 0;
	if (ItemStack.DungeonAttributes.IsEmpty()) return TryStoreItem(ItemStack.ItemId, ItemStack.Quantity, ItemStack.TotalQuality);

	const FItemDefinitionRow* Def = FindItemDefinition(ItemStack.ItemId);
	if (!Def || ItemStack.Quantity > Def->StackLimit || !CanStoreItemStacks({ ItemStack })) return 0;
	StoredItems.Add(ItemStack);
	BroadcastChanged();
	return ItemStack.Quantity;
}

int32 UWS_ItemStorage::TryTakeStoredItem(FName ItemId, int32 RequestedQuantity, float& OutRemovedQuality)
{
	OutRemovedQuality = 0.f;
	int32 Remaining = FMath::Max(0, RequestedQuantity);
	int32 Removed = 0;
	for (int32 Index = StoredItems.Num() - 1; Index >= 0 && Remaining > 0; --Index)
	{
		FItemStack& Stack = StoredItems[Index];
		if (Stack.ItemId != ItemId)
		{
			continue;
		}
		const int32 ToRemove = FMath::Min(Remaining, Stack.Quantity);
		const float QualityToRemove = Stack.Quantity > 0 ? Stack.TotalQuality * (static_cast<float>(ToRemove) / Stack.Quantity) : 0.f;
		Stack.Quantity -= ToRemove;
		Stack.TotalQuality -= QualityToRemove;
		Remaining -= ToRemove;
		Removed += ToRemove;
		OutRemovedQuality += QualityToRemove;
		if (Stack.Quantity <= 0)
		{
			StoredItems.RemoveAt(Index);
		}
	}
	if (Removed > 0)
	{
		BroadcastChanged();
	}
	return Removed;
}

bool UWS_ItemStorage::TryTakeStoredItemStack(const FItemStack& ItemStack)
{
	for (int32 Index = 0; Index < StoredItems.Num(); ++Index)
	{
		if (StoredItems[Index].ItemId == ItemStack.ItemId && StoredItems[Index].Quantity == ItemStack.Quantity
			&& HasSameDungeonAttributes(StoredItems[Index], ItemStack))
		{
			StoredItems.RemoveAt(Index);
			BroadcastChanged();
			return true;
		}
	}
	return false;
}

void UWS_ItemStorage::DepositPageInventory(APageCharacter* Page)
{
	UInventoryComponent* Inventory = Page ? Page->GetInventory() : nullptr;
	UWS_Economy* Economy = GetWorld() ? GetWorld()->GetSubsystem<UWS_Economy>() : nullptr;
	if (!Inventory || !Economy)
	{
		return;
	}

	const TArray<FItemStack> CarriedStacks = Inventory->GetStacks();
	for (const FItemStack& Stack : CarriedStacks)
	{
		const FItemDefinitionRow* Def = FindItemDefinition(Stack.ItemId);
		if (!Def)
		{
			continue;
		}

		const int32 Accepted = Def->bConvertOnReturn && !Def->SettlementResourceId.IsNone()
			? Economy->TryAddAmount(Def->SettlementResourceId, Stack.Quantity)
			: TryStoreItemStack(Stack);
		if (Accepted > 0)
		{
			if (!Stack.DungeonAttributes.IsEmpty()) Inventory->TryRemoveItemStack(Stack);
			else { float IgnoredQuality = 0.f; Inventory->TryRemoveItem(Stack.ItemId, Accepted, IgnoredQuality); }
		}
	}
}

bool UWS_ItemStorage::HasCompletedEPAltar() const
{
	UWS_Building* Buildings = GetWorld() ? GetWorld()->GetSubsystem<UWS_Building>() : nullptr;
	if (!Buildings) return false;
	TArray<FName> CompletedBuildingIds;
	Buildings->GetCompletedBuildingIds(CompletedBuildingIds);
	return CompletedBuildingIds.Contains(TEXT("EP_Altar"));
}

bool UWS_ItemStorage::OfferPortalShardAtIndex(int32 StoredStackIndex)
{
	if (!StoredItems.IsValidIndex(StoredStackIndex) || !HasCompletedEPAltar()) return false;
	const FItemStack& Shard = StoredItems[StoredStackIndex];
	if (Shard.ItemId != TEXT("PortalShard") || Shard.DungeonAttributes.IsEmpty()) return false;

	UWS_Economy* Economy = GetWorld() ? GetWorld()->GetSubsystem<UWS_Economy>() : nullptr;
	if (!Economy) return false;
	for (const FDungeonAttributeWeight& Attribute : Shard.DungeonAttributes)
	{
		const FName ReserveId(*FString::Printf(TEXT("%s_Attribute"), *Attribute.AttributeId.ToString()));
		if (Attribute.Strength <= 0 || !FindResourceDefinition(ReserveId)) return false;
	}

	const FItemStack OfferedShard = Shard;
	StoredItems.RemoveAt(StoredStackIndex);
	for (const FDungeonAttributeWeight& Attribute : OfferedShard.DungeonAttributes)
	{
		const FName ReserveId(*FString::Printf(TEXT("%s_Attribute"), *Attribute.AttributeId.ToString()));
		Economy->AddAmount(ReserveId, Attribute.Strength);
	}
	BroadcastChanged();
	UE_LOG(LogTemp, Log, TEXT("[EP Altar] Absorbed PortalShard with %d attribute components"), OfferedShard.DungeonAttributes.Num());
	return true;
}

void UWS_ItemStorage::ConvertReturnResources(APageCharacter* Page)
{
	UInventoryComponent* Inventory = Page ? Page->GetInventory() : nullptr;
	UWS_Economy* Economy = GetWorld() ? GetWorld()->GetSubsystem<UWS_Economy>() : nullptr;
	if (!Inventory || !Economy)
	{
		return;
	}

	const TArray<FItemStack> CarriedStacks = Inventory->GetStacks();
	for (const FItemStack& Stack : CarriedStacks)
	{
		const FItemDefinitionRow* Def = FindItemDefinition(Stack.ItemId);
		if (!Def || !Def->bConvertOnReturn || Def->SettlementResourceId.IsNone())
		{
			continue;
		}

		const int32 Converted = Economy->TryAddAmount(Def->SettlementResourceId, Stack.Quantity);
		if (Converted > 0)
		{
			float IgnoredQuality = 0.f;
			Inventory->TryRemoveItem(Stack.ItemId, Converted, IgnoredQuality);
		}
	}
}

void UWS_ItemStorage::NotifyResourceChanged()
{
	BroadcastChanged();
}

void UWS_ItemStorage::WriteToSnapshot_Implementation(FEidosWorldSnapshot& OutSnapshot) const
{
	OutSnapshot.SetKVString(SnapshotKey(), EncodeStacks(StoredItems));
}

void UWS_ItemStorage::ApplySnapshot_Implementation(const FEidosWorldSnapshot& Snapshot)
{
	DecodeStacks(Snapshot.GetKVString(SnapshotKey()), StoredItems);
	BroadcastChanged();
}

FString UWS_ItemStorage::EncodeStacks(const TArray<FItemStack>& Stacks)
{
	TArray<FString> Entries;
	for (const FItemStack& Stack : Stacks)
	{
		if (Stack.IsValid())
		{
			Entries.Add(FString::Printf(TEXT("%s,%d,%.6f,%s"), *Stack.ItemId.ToString(), Stack.Quantity, Stack.TotalQuality, *EncodeDungeonAttributes(Stack.DungeonAttributes)));
		}
	}
	return FString::Join(Entries, TEXT(";"));
}

void UWS_ItemStorage::DecodeStacks(const FString& Encoded, TArray<FItemStack>& OutStacks)
{
	OutStacks.Reset();
	TArray<FString> Entries;
	Encoded.ParseIntoArray(Entries, TEXT(";"), true);
	for (const FString& Entry : Entries)
	{
		TArray<FString> Fields;
		Entry.ParseIntoArray(Fields, TEXT(","), false);
		if (Fields.Num() < 3)
		{
			continue;
		}
		FItemStack Stack;
		Stack.ItemId = FName(*Fields[0]);
		Stack.Quantity = FCString::Atoi(*Fields[1]);
		Stack.TotalQuality = FCString::Atof(*Fields[2]);
		if (Fields.Num() >= 4) DecodeDungeonAttributes(Fields[3], Stack.DungeonAttributes);
		if (Stack.IsValid())
		{
			OutStacks.Add(Stack);
		}
	}
}

void UWS_ItemStorage::BroadcastChanged()
{
	OnStorageChanged.Broadcast();
}
