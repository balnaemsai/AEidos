// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/Types/ItemTypes.h"
#include "UI/HUD/Panels/PanelLifeCycle.h"
#include "Panel_Items.generated.h"

class UTextBlock;
class UVerticalBox;
class UButton;
class UItemTransferEntry;
class UItemContextMenuWidget;

USTRUCT()
struct FItemTransferSelection
{
	GENERATED_BODY()
	FName ItemId = NAME_None;
	bool bFromStorage = false;
	bool Matches(FName InItemId, bool bInFromStorage) const { return ItemId == InItemId && bFromStorage == bInFromStorage; }
};

USTRUCT(BlueprintType)
struct FStoredResourceView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FName ResourceId = NAME_None;

	UPROPERTY(BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly)
	int32 Amount = 0;
};

UCLASS()
class AEIDOS_API UPanel_Items : public UUserWidget, public IPanelLifecycle
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void OnPanelShown_Implementation() override;
	virtual void OnPanelHidden_Implementation() override;

	UFUNCTION(BlueprintCallable, Category="Items")
	void RefreshFromWorld();

	UFUNCTION(BlueprintPure, Category="Items")
	const TArray<FStoredResourceView>& GetResourceViews() const { return CachedResourceViews; }

	UFUNCTION(BlueprintPure, Category="Items")
	const TArray<FItemStack>& GetStoredItems() const { return CachedStoredItems; }

	UFUNCTION(BlueprintPure, Category="Items")
	const TArray<FItemStack>& GetSelectedPageItems() const { return CachedSelectedPageItems; }

	UFUNCTION(BlueprintCallable, Category="Items") bool StoreSelectedItem(int32 RequestedQuantity);
	UFUNCTION(BlueprintCallable, Category="Items") bool TakeSelectedItem(int32 RequestedQuantity);

	/** Extension hook for data-defined actions such as Use or Equip. */
	UFUNCTION(BlueprintImplementableEvent, Category="Items")
	void OnInventoryItemActionRequested(EInventoryItemActionType Action, const TArray<FName>& ItemIds);

protected:
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UVerticalBox> VerticalBox_ResourceEntries;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UVerticalBox> VerticalBox_StoredItemEntries;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UVerticalBox> VerticalBox_PageItemEntries;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_StorageWeight;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_StorageVolume;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_SelectedPageInventory;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_EmptyStoredItems;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_EmptyPageItems;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_SelectedTransfer;

	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_StoreOne;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_StoreAll;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_TakeOne;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_TakeAll;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_Close;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_OpenWorkOrders;

	UPROPERTY(EditDefaultsOnly, Category="Items") TSubclassOf<UItemTransferEntry> ItemTransferEntryClass;
	UPROPERTY(EditDefaultsOnly, Category="Items") TSubclassOf<UItemContextMenuWidget> ItemContextMenuClass;

	UPROPERTY(EditDefaultsOnly, Category="Items", meta=(ClampMin="0.1"))
	float RefreshIntervalSeconds = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category="Items")
	TArray<FStoredResourceView> CachedResourceViews;

	UPROPERTY(BlueprintReadOnly, Category="Items")
	TArray<FItemStack> CachedStoredItems;

	UPROPERTY(BlueprintReadOnly, Category="Items")
	TArray<FItemStack> CachedSelectedPageItems;

private:
	void StartAutoRefresh();
	void StopAutoRefresh();
	void RebuildListWidgets();
	void RefreshSelectedTransferText();
	void ValidateSelection();
	FText GetItemDisplayName(FName ItemId) const;
	const FItemStack* FindSelectedStack() const;
	const FItemStack* FindStack(FName ItemId, bool bFromStorage) const;
	bool IsSelected(FName ItemId, bool bFromStorage) const;
	bool MoveSelectedItems(bool bToStorage);
	bool DropSelectedItems();
	bool UseSelectedItems();
	bool StartPlacementForSelectedItem();
	bool EquipSelectedItem();
	bool SelectionSupportsAction(EInventoryItemActionType Action) const;
	void OpenItemContextMenu();
	void CloseItemContextMenu();
	FText GetSelectionSummary() const;
	FText GetActionLabel(EInventoryItemActionType Action) const;

	UFUNCTION() void HandleTransferEntrySelected(FName ItemId, bool bFromStorage, bool bAdditive);
	UFUNCTION() void HandleTransferEntryContextRequested(FName ItemId, bool bFromStorage);
	UFUNCTION() void HandleContextActionSelected(EInventoryItemActionType Action);
	UFUNCTION() void HandleStoreOneClicked();
	UFUNCTION() void HandleStoreAllClicked();
	UFUNCTION() void HandleTakeOneClicked();
	UFUNCTION() void HandleTakeAllClicked();
	UFUNCTION() void HandleCloseClicked();
	UFUNCTION() void HandleOpenWorkOrdersClicked();

	TArray<FItemTransferSelection> SelectedTransferItems;
	TObjectPtr<UItemContextMenuWidget> ActiveItemContextMenu;
	FTimerHandle RefreshTimerHandle;
};
