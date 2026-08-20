#include "UI/HUD/Panels/Panel_Items.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Data/Definitions/ItemDefinitionRow.h"
#include "Data/Definitions/ResourceDefinitionRow.h"
#include "Data/GIS_DataRegistry.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Entities/Items/InventoryComponent.h"
#include "Entities/Items/EquipmentComponent.h"
#include "Entities/Page/PageCharacter.h"
#include "Entities/Page/Components/StatsComponent.h"
#include "Framework/EidosPlayerController.h"
#include "UI/HUD/PanelUIFunctionLibrary.h"
#include "UI/HUD/HUDRootWidget.h"
#include "UI/HUD/Panels/ItemContextMenuWidget.h"
#include "UI/HUD/Panels/ItemTransferEntry.h"
#include "World/Interaction/WorldItemBlockActor.h"
#include "World/Settlement/WS_Economy.h"
#include "World/Settlement/WS_ItemStorage.h"

void UPanel_Items::NativeConstruct()
{
	Super::NativeConstruct();
	if (!ItemTransferEntryClass)
	{
		ItemTransferEntryClass = LoadClass<UItemTransferEntry>(nullptr, TEXT("/Game/Blueprints/WBP/WBP_ItemTransferEntry.WBP_ItemTransferEntry_C"));
	}
	if (!ItemContextMenuClass)
	{
		ItemContextMenuClass = LoadClass<UItemContextMenuWidget>(nullptr, TEXT("/Game/Blueprints/WBP/WBP_ItemContextMenu.WBP_ItemContextMenu_C"));
	}
	if (Button_StoreOne) Button_StoreOne->OnClicked.AddDynamic(this, &UPanel_Items::HandleStoreOneClicked);
	if (Button_StoreAll) Button_StoreAll->OnClicked.AddDynamic(this, &UPanel_Items::HandleStoreAllClicked);
	if (Button_TakeOne) Button_TakeOne->OnClicked.AddDynamic(this, &UPanel_Items::HandleTakeOneClicked);
	if (Button_TakeAll) Button_TakeAll->OnClicked.AddDynamic(this, &UPanel_Items::HandleTakeAllClicked);
	if (Button_Close) Button_Close->OnClicked.AddDynamic(this, &UPanel_Items::HandleCloseClicked);
	if (Button_OpenWorkOrders) Button_OpenWorkOrders->OnClicked.AddDynamic(this, &UPanel_Items::HandleOpenWorkOrdersClicked);
	RefreshFromWorld();
}

void UPanel_Items::NativeDestruct()
{
	CloseItemContextMenu();
	StopAutoRefresh();
	Super::NativeDestruct();
}

void UPanel_Items::OnPanelShown_Implementation()
{
	RefreshFromWorld();
	StartAutoRefresh();
}

void UPanel_Items::OnPanelHidden_Implementation()
{
	CloseItemContextMenu();
	StopAutoRefresh();
}

void UPanel_Items::RefreshFromWorld()
{
	CachedResourceViews.Reset();
	CachedStoredItems.Reset();
	CachedSelectedPageItems.Reset();
	UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	UGIS_DataRegistry* Registry = GameInstance ? GameInstance->GetSubsystem<UGIS_DataRegistry>() : nullptr;
	UWS_Economy* Economy = World ? World->GetSubsystem<UWS_Economy>() : nullptr;
	UWS_ItemStorage* Storage = World ? World->GetSubsystem<UWS_ItemStorage>() : nullptr;
	if (!Registry || !Registry->EnsureReadySync() || !Economy || !Storage)
	{
		RebuildListWidgets();
		return;
	}

	for (const FName ResourceId : Registry->GetAllResourceIds())
	{
		if (const FResourceDefinitionRow* Def = Registry->GetResourceDef(ResourceId))
		{
			FStoredResourceView& View = CachedResourceViews.AddDefaulted_GetRef();
			View.ResourceId = ResourceId;
			View.DisplayName = Def->DisplayName;
			View.Amount = Economy->GetAmount(ResourceId);
		}
	}
	CachedResourceViews.Sort([](const FStoredResourceView& A, const FStoredResourceView& B) { return A.ResourceId.LexicalLess(B.ResourceId); });
	CachedStoredItems = Storage->GetStoredItems();
	if (const AEidosPlayerController* PC = Cast<AEidosPlayerController>(GetOwningPlayer()))
	{
		if (const APageCharacter* SelectedPage = PC->GetSelectedPage())
		{
			if (const UInventoryComponent* Inventory = SelectedPage->GetInventory()) CachedSelectedPageItems = Inventory->GetStacks();
		}
	}
	ValidateSelection();

	if (Text_StorageWeight) Text_StorageWeight->SetText(FText::Format(FText::FromString(TEXT("Warehouse weight {0} / {1}")), FMath::RoundToInt(Storage->GetCurrentWeight()), FMath::RoundToInt(Storage->GetTotalWeightCapacity())));
	if (Text_StorageVolume) Text_StorageVolume->SetText(FText::Format(FText::FromString(TEXT("Warehouse volume {0} / {1}")), FMath::RoundToInt(Storage->GetCurrentVolume()), FMath::RoundToInt(Storage->GetTotalVolumeCapacity())));
	if (Text_SelectedPageInventory)
	{
		const AEidosPlayerController* PC = Cast<AEidosPlayerController>(GetOwningPlayer());
		const APageCharacter* SelectedPage = PC ? PC->GetSelectedPage() : nullptr;
		Text_SelectedPageInventory->SetText(SelectedPage
			? FText::Format(FText::FromString(TEXT("{0} cargo  {1}/{2} volume  {3}/{4} weight")), FText::FromString(SelectedPage->GetName()), FMath::RoundToInt(SelectedPage->GetCurrentInventoryVolume()), FMath::RoundToInt(SelectedPage->GetMaxInventoryVolume()), FMath::RoundToInt(SelectedPage->GetCurrentInventoryWeight()), FMath::RoundToInt(SelectedPage->GetMaxInventoryWeight()))
			: FText::FromString(TEXT("No Page selected")));
	}
	RebuildListWidgets();
}

bool UPanel_Items::StoreSelectedItem(int32 RequestedQuantity)
{
	return MoveSelectedItems(true);
}

bool UPanel_Items::TakeSelectedItem(int32 RequestedQuantity)
{
	return MoveSelectedItems(false);
}

void UPanel_Items::StartAutoRefresh()
{
	if (UWorld* World = GetWorld()) World->GetTimerManager().SetTimer(RefreshTimerHandle, this, &UPanel_Items::RefreshFromWorld, RefreshIntervalSeconds, true);
}

void UPanel_Items::StopAutoRefresh()
{
	if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(RefreshTimerHandle);
}

void UPanel_Items::RebuildListWidgets()
{
	auto AddLine = [](UVerticalBox* Box, const FText& Text)
	{
		if (!Box) return;
		UTextBlock* Line = NewObject<UTextBlock>(Box);
		Line->SetText(Text);
		Line->SetColorAndOpacity(FSlateColor(FLinearColor(0.84f, 0.86f, 0.86f)));
		Line->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 14));
		Box->AddChildToVerticalBox(Line);
	};
	if (VerticalBox_ResourceEntries)
	{
		VerticalBox_ResourceEntries->ClearChildren();
		for (const FStoredResourceView& Resource : CachedResourceViews) AddLine(VerticalBox_ResourceEntries, FText::Format(FText::FromString(TEXT("{0}  {1}")), Resource.DisplayName, FText::AsNumber(Resource.Amount)));
	}
	auto RebuildItemList = [this, &AddLine](UVerticalBox* Box, const TArray<FItemStack>& Stacks, bool bFromStorage)
	{
		if (!Box) return;
		Box->ClearChildren();
		for (const FItemStack& Stack : Stacks)
		{
			if (ItemTransferEntryClass)
			{
				if (UItemTransferEntry* Entry = CreateWidget<UItemTransferEntry>(this, ItemTransferEntryClass))
				{
					Entry->Setup(Stack.ItemId, GetItemDisplayName(Stack.ItemId), Stack.Quantity, bFromStorage, IsSelected(Stack.ItemId, bFromStorage));
					Entry->OnEntrySelected.AddDynamic(this, &UPanel_Items::HandleTransferEntrySelected);
					Entry->OnEntryContextRequested.AddDynamic(this, &UPanel_Items::HandleTransferEntryContextRequested);
					Box->AddChildToVerticalBox(Entry);
				}
			}
			else AddLine(Box, FText::Format(FText::FromString(TEXT("{0} x{1}")), FText::FromName(Stack.ItemId), FText::AsNumber(Stack.Quantity)));
		}
	};
	RebuildItemList(VerticalBox_StoredItemEntries, CachedStoredItems, true);
	RebuildItemList(VerticalBox_PageItemEntries, CachedSelectedPageItems, false);
	if (Text_EmptyStoredItems) Text_EmptyStoredItems->SetVisibility(CachedStoredItems.IsEmpty() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (Text_EmptyPageItems) Text_EmptyPageItems->SetVisibility(CachedSelectedPageItems.IsEmpty() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	RefreshSelectedTransferText();
}

void UPanel_Items::RefreshSelectedTransferText()
{
	if (Text_SelectedTransfer) Text_SelectedTransfer->SetText(GetSelectionSummary());
}

void UPanel_Items::ValidateSelection()
{
	SelectedTransferItems.RemoveAll([this](const FItemTransferSelection& Selection) { return !FindStack(Selection.ItemId, Selection.bFromStorage); });
}

FText UPanel_Items::GetItemDisplayName(FName ItemId) const
{
	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UGIS_DataRegistry* Registry = GameInstance ? GameInstance->GetSubsystem<UGIS_DataRegistry>() : nullptr;
	if (const FItemDefinitionRow* Def = Registry ? Registry->GetItemDef(ItemId) : nullptr) return Def->DisplayName;
	return FText::FromName(ItemId);
}

const FItemStack* UPanel_Items::FindStack(FName ItemId, bool bFromStorage) const
{
	const TArray<FItemStack>& Stacks = bFromStorage ? CachedStoredItems : CachedSelectedPageItems;
	return Stacks.FindByPredicate([ItemId](const FItemStack& Stack) { return Stack.ItemId == ItemId; });
}

const FItemStack* UPanel_Items::FindSelectedStack() const
{
	return SelectedTransferItems.Num() == 1 ? FindStack(SelectedTransferItems[0].ItemId, SelectedTransferItems[0].bFromStorage) : nullptr;
}

bool UPanel_Items::IsSelected(FName ItemId, bool bFromStorage) const
{
	return SelectedTransferItems.ContainsByPredicate([ItemId, bFromStorage](const FItemTransferSelection& Selection) { return Selection.Matches(ItemId, bFromStorage); });
}

bool UPanel_Items::MoveSelectedItems(bool bToStorage)
{
	if (SelectedTransferItems.IsEmpty()) return false;
	const bool bSourceIsStorage = SelectedTransferItems[0].bFromStorage;
	if (bToStorage == bSourceIsStorage) return false;
	UWorld* World = GetWorld();
	AEidosPlayerController* PC = Cast<AEidosPlayerController>(GetOwningPlayer());
	UInventoryComponent* Inventory = PC && PC->GetSelectedPage() ? PC->GetSelectedPage()->GetInventory() : nullptr;
	UWS_ItemStorage* Storage = World ? World->GetSubsystem<UWS_ItemStorage>() : nullptr;
	if (!Inventory || !Storage) return false;

	bool bMovedAny = false;
	const TArray<FItemTransferSelection> Selections = SelectedTransferItems;
	for (const FItemTransferSelection& Selection : Selections)
	{
		const FItemStack* Stack = FindStack(Selection.ItemId, bSourceIsStorage);
		if (!Stack) continue;
		const FItemStack StackCopy = *Stack;
		if (bToStorage)
		{
			const int32 Stored = Storage->TryStoreItemStack(StackCopy);
			if (Stored > 0)
			{
				float RemovedQuality = 0.f;
				const int32 Removed = StackCopy.DungeonAttributes.IsEmpty()
					? Inventory->TryRemoveItem(StackCopy.ItemId, Stored, RemovedQuality)
					: (Inventory->TryRemoveItemStack(StackCopy) ? StackCopy.Quantity : 0);
				if (Removed != Stored) { if (!StackCopy.DungeonAttributes.IsEmpty()) Storage->TryTakeStoredItemStack(StackCopy); else { float Ignored = 0.f; Storage->TryTakeStoredItem(StackCopy.ItemId, Stored - Removed, Ignored); } }
				bMovedAny |= Removed > 0;
			}
		}
		else
		{
			float TakenQuality = 0.f;
			const int32 Taken = StackCopy.DungeonAttributes.IsEmpty()
				? Storage->TryTakeStoredItem(StackCopy.ItemId, StackCopy.Quantity, TakenQuality)
				: (Storage->TryTakeStoredItemStack(StackCopy) ? StackCopy.Quantity : 0);
			if (Taken > 0)
			{
				const int32 Added = StackCopy.DungeonAttributes.IsEmpty() ? Inventory->TryAddItem(StackCopy.ItemId, Taken, TakenQuality) : Inventory->TryAddItemStack(StackCopy);
				if (Added != Taken) { if (!StackCopy.DungeonAttributes.IsEmpty()) Storage->TryStoreItemStack(StackCopy); else Storage->TryStoreItem(StackCopy.ItemId, Taken - Added, Taken > 0 ? TakenQuality * static_cast<float>(Taken - Added) / Taken : 0.f); }
				bMovedAny |= Added > 0;
			}
		}
	}
	if (bMovedAny) { SelectedTransferItems.Reset(); RefreshFromWorld(); }
	return bMovedAny;
}

bool UPanel_Items::DropSelectedItems()
{
	if (SelectedTransferItems.IsEmpty() || SelectedTransferItems[0].bFromStorage) return false;
	AEidosPlayerController* PC = Cast<AEidosPlayerController>(GetOwningPlayer());
	APageCharacter* Page = PC ? PC->GetSelectedPage() : nullptr;
	UInventoryComponent* Inventory = Page ? Page->GetInventory() : nullptr;
	UGIS_DataRegistry* Registry = GetWorld() && GetWorld()->GetGameInstance() ? GetWorld()->GetGameInstance()->GetSubsystem<UGIS_DataRegistry>() : nullptr;
	if (!Page || !Inventory || !Registry) return false;
	bool bDroppedAny = false;
	for (const FItemTransferSelection& Selection : SelectedTransferItems)
	{
		const FItemStack* Stack = FindStack(Selection.ItemId, false);
		const FItemDefinitionRow* Def = Stack ? Registry->GetItemDef(Stack->ItemId) : nullptr;
		UClass* PickupClass = Def ? Def->WorldPickupClass.LoadSynchronous() : nullptr;
		if (!Stack || !PickupClass || !PickupClass->IsChildOf(AWorldItemBlockActor::StaticClass())) continue;
		const FVector Location = Page->GetActorLocation() + Page->GetActorForwardVector() * 90.f + FVector(0.f, 0.f, 30.f);
		AWorldItemBlockActor* WorldItem = GetWorld()->SpawnActor<AWorldItemBlockActor>(PickupClass, Location, Page->GetActorRotation());
		if (!WorldItem) continue;
		WorldItem->InitializeWorldItem(Stack->ItemId, Stack->Quantity);
		float RemovedQuality = 0.f;
		if (Inventory->TryRemoveItem(Stack->ItemId, Stack->Quantity, RemovedQuality) <= 0) { WorldItem->Destroy(); continue; }
		bDroppedAny = true;
	}
	if (bDroppedAny) { SelectedTransferItems.Reset(); RefreshFromWorld(); }
	return bDroppedAny;
}

bool UPanel_Items::UseSelectedItems()
{
	if (SelectedTransferItems.IsEmpty() || SelectedTransferItems[0].bFromStorage)
	{
		return false;
	}

	AEidosPlayerController* PC = Cast<AEidosPlayerController>(GetOwningPlayer());
	APageCharacter* Page = PC ? PC->GetSelectedPage() : nullptr;
	UInventoryComponent* Inventory = Page ? Page->GetInventory() : nullptr;
	UStatsComponent* Stats = Page ? Page->FindComponentByClass<UStatsComponent>() : nullptr;
	UGIS_DataRegistry* Registry = GetWorld() && GetWorld()->GetGameInstance()
		? GetWorld()->GetGameInstance()->GetSubsystem<UGIS_DataRegistry>()
		: nullptr;
	if (!Page || !Inventory || !Registry)
	{
		return false;
	}

	bool bUsedAny = false;
	TArray<FName> CustomUseItemIds;
	for (const FItemTransferSelection& Selection : SelectedTransferItems)
	{
		const FItemStack* Stack = FindStack(Selection.ItemId, false);
		const FItemDefinitionRow* Definition = Stack ? Registry->GetItemDef(Stack->ItemId) : nullptr;
		if (!Stack || !Definition || Stack->Quantity <= 0)
		{
			continue;
		}

		bool bApplied = false;
		switch (Definition->UseEffect)
		{
		case EItemUseEffectType::RestoreHealth:
			bApplied = Stats && Stats->RestoreHealth(Definition->UseEffectMagnitude) > 0.f;
			break;
		case EItemUseEffectType::None:
			CustomUseItemIds.Add(Stack->ItemId);
			break;
		default:
			break;
		}

		if (!bApplied)
		{
			continue;
		}

		if (Definition->bConsumeOnUse)
		{
			float RemovedQuality = 0.f;
			bApplied = Inventory->TryRemoveItem(Stack->ItemId, 1, RemovedQuality) == 1;
		}
		bUsedAny |= bApplied;
	}

	if (!CustomUseItemIds.IsEmpty())
	{
		// Blueprint remains the extension point for effects not yet represented by EItemUseEffectType.
		OnInventoryItemActionRequested(EInventoryItemActionType::Use, CustomUseItemIds);
	}
	if (bUsedAny)
	{
		SelectedTransferItems.Reset();
		RefreshFromWorld();
	}
	return bUsedAny;
}

bool UPanel_Items::StartPlacementForSelectedItem()
{
	if (SelectedTransferItems.Num() != 1 || SelectedTransferItems[0].bFromStorage) return false;
	AEidosPlayerController* PC = Cast<AEidosPlayerController>(GetOwningPlayer());
	if (!PC || !PC->BeginBlockPlacement(SelectedTransferItems[0].ItemId)) return false;
	CloseItemContextMenu();
	UPanelUIFunctionLibrary::ClosePanel(this);
	return true;
}

bool UPanel_Items::EquipSelectedItem()
{
	if (SelectedTransferItems.Num() != 1 || SelectedTransferItems[0].bFromStorage) return false;
	AEidosPlayerController* PC = Cast<AEidosPlayerController>(GetOwningPlayer());
	APageCharacter* Page = PC ? PC->GetSelectedPage() : nullptr;
	UGIS_DataRegistry* Registry = GetWorld() && GetWorld()->GetGameInstance() ? GetWorld()->GetGameInstance()->GetSubsystem<UGIS_DataRegistry>() : nullptr;
	const FItemDefinitionRow* Def = Registry ? Registry->GetItemDef(SelectedTransferItems[0].ItemId) : nullptr;
	if (!Page || !Def || Def->CompatibleEquipmentSlots.Num() != 1) return false;
	if (!Page->GetEquipment() || !Page->GetEquipment()->EquipFromInventory(SelectedTransferItems[0].ItemId, Def->CompatibleEquipmentSlots[0])) return false;
	SelectedTransferItems.Reset();
	RefreshFromWorld();
	return true;
}

bool UPanel_Items::SelectionSupportsAction(EInventoryItemActionType Action) const
{
	if (SelectedTransferItems.IsEmpty()) return false;
	UGIS_DataRegistry* Registry = GetWorld() && GetWorld()->GetGameInstance() ? GetWorld()->GetGameInstance()->GetSubsystem<UGIS_DataRegistry>() : nullptr;
	if (!Registry) return false;
	for (const FItemTransferSelection& Selection : SelectedTransferItems)
	{
		const FItemDefinitionRow* Def = Registry->GetItemDef(Selection.ItemId);
		if (!Def || !Def->InventoryActions.Contains(Action)) return false;
		if (Action == EInventoryItemActionType::Place && (!Def->PlacedBlockClass.IsValid() && Def->PlacedBlockClass.IsNull())) return false;
	}
	return true;
}

FText UPanel_Items::GetSelectionSummary() const
{
	if (SelectedTransferItems.IsEmpty()) return FText::FromString(TEXT("Select items"));
	return SelectedTransferItems.Num() == 1
		? FText::Format(FText::FromString(TEXT("{0} selected")), GetItemDisplayName(SelectedTransferItems[0].ItemId))
		: FText::Format(FText::FromString(TEXT("{0} items selected")), FText::AsNumber(SelectedTransferItems.Num()));
}

FText UPanel_Items::GetActionLabel(EInventoryItemActionType Action) const
{
	switch (Action)
	{
	case EInventoryItemActionType::MoveToOtherInventory: return SelectedTransferItems[0].bFromStorage ? FText::FromString(TEXT("Move to Page")) : FText::FromString(TEXT("Move to Warehouse"));
	case EInventoryItemActionType::Drop: return FText::FromString(TEXT("Drop"));
	case EInventoryItemActionType::Use: return FText::FromString(TEXT("Use"));
	case EInventoryItemActionType::Place: return FText::FromString(TEXT("Place"));
	case EInventoryItemActionType::Equip: return FText::FromString(TEXT("Equip"));
	default: return FText::GetEmpty();
	}
}

void UPanel_Items::OpenItemContextMenu()
{
	if (SelectedTransferItems.IsEmpty() || !ItemContextMenuClass) return;
	CloseItemContextMenu();
	TArray<FItemContextActionView> Actions;
	Actions.Add({ EInventoryItemActionType::MoveToOtherInventory, GetActionLabel(EInventoryItemActionType::MoveToOtherInventory) });
	bool bCanDrop = !SelectedTransferItems[0].bFromStorage;
	UGIS_DataRegistry* Registry = GetWorld() && GetWorld()->GetGameInstance() ? GetWorld()->GetGameInstance()->GetSubsystem<UGIS_DataRegistry>() : nullptr;
	for (const FItemTransferSelection& Selection : SelectedTransferItems)
	{
		const FItemDefinitionRow* Def = Registry ? Registry->GetItemDef(Selection.ItemId) : nullptr;
		UClass* PickupClass = Def ? Def->WorldPickupClass.LoadSynchronous() : nullptr;
		bCanDrop &= PickupClass && PickupClass->IsChildOf(AWorldItemBlockActor::StaticClass());
	}
	if (bCanDrop) Actions.Add({ EInventoryItemActionType::Drop, GetActionLabel(EInventoryItemActionType::Drop) });
	for (const EInventoryItemActionType Action : { EInventoryItemActionType::Use, EInventoryItemActionType::Place, EInventoryItemActionType::Equip })
	{
		if (SelectionSupportsAction(Action) && ((Action != EInventoryItemActionType::Place && Action != EInventoryItemActionType::Equip) || SelectedTransferItems.Num() == 1)) Actions.Add({ Action, GetActionLabel(Action) });
	}
	ActiveItemContextMenu = CreateWidget<UItemContextMenuWidget>(GetOwningPlayer(), ItemContextMenuClass);
	if (!ActiveItemContextMenu) return;
	ActiveItemContextMenu->OnActionSelected.AddDynamic(this, &UPanel_Items::HandleContextActionSelected);
	float X = 0.f, Y = 0.f;
	if (APlayerController* PC = GetOwningPlayer()) PC->GetMousePosition(X, Y);
	ActiveItemContextMenu->AddToViewport(20);
	ActiveItemContextMenu->ShowMenu(GetSelectionSummary(), Actions, FVector2D(X, Y));
}

void UPanel_Items::CloseItemContextMenu()
{
	if (ActiveItemContextMenu) ActiveItemContextMenu->RemoveFromParent();
	ActiveItemContextMenu = nullptr;
}

void UPanel_Items::HandleTransferEntrySelected(FName ItemId, bool bFromStorage, bool bAdditive)
{
	CloseItemContextMenu();
	const int32 Existing = SelectedTransferItems.IndexOfByPredicate([ItemId, bFromStorage](const FItemTransferSelection& Selection) { return Selection.Matches(ItemId, bFromStorage); });
	if (!bAdditive || (!SelectedTransferItems.IsEmpty() && SelectedTransferItems[0].bFromStorage != bFromStorage))
	{
		SelectedTransferItems.Reset();
		SelectedTransferItems.Add({ ItemId, bFromStorage });
	}
	else if (Existing != INDEX_NONE) SelectedTransferItems.RemoveAt(Existing);
	else SelectedTransferItems.Add({ ItemId, bFromStorage });
	RebuildListWidgets();
}

void UPanel_Items::HandleTransferEntryContextRequested(FName ItemId, bool bFromStorage)
{
	if (!IsSelected(ItemId, bFromStorage))
	{
		SelectedTransferItems.Reset();
		SelectedTransferItems.Add({ ItemId, bFromStorage });
		RebuildListWidgets();
	}
	OpenItemContextMenu();
}

void UPanel_Items::HandleContextActionSelected(EInventoryItemActionType Action)
{
	switch (Action)
	{
	case EInventoryItemActionType::MoveToOtherInventory: MoveSelectedItems(!SelectedTransferItems[0].bFromStorage); break;
	case EInventoryItemActionType::Drop: DropSelectedItems(); break;
	case EInventoryItemActionType::Place: StartPlacementForSelectedItem(); return;
	case EInventoryItemActionType::Equip: EquipSelectedItem(); break;
	case EInventoryItemActionType::Use: UseSelectedItems(); break;
	default: break;
	}
	CloseItemContextMenu();
}

void UPanel_Items::HandleStoreOneClicked() { MoveSelectedItems(true); }
void UPanel_Items::HandleStoreAllClicked() { MoveSelectedItems(true); }
void UPanel_Items::HandleTakeOneClicked() { MoveSelectedItems(false); }
void UPanel_Items::HandleTakeAllClicked() { MoveSelectedItems(false); }
void UPanel_Items::HandleCloseClicked() { CloseItemContextMenu(); UPanelUIFunctionLibrary::ClosePanel(this); }
void UPanel_Items::HandleOpenWorkOrdersClicked()
{
	if (UHUDRootWidget* HUDRoot = GetTypedOuter<UHUDRootWidget>())
	{
		HUDRoot->ShowWorkOrderPopup();
	}
}
