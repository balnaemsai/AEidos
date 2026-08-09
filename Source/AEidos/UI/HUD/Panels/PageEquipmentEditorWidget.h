#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/Types/ItemTypes.h"
#include "PageEquipmentEditorWidget.generated.h"

class APageCharacter;
class UButton;
class UEquipmentComponent;
class UInventoryComponent;
class UPageEquipmentItemEntry;
class UTextBlock;
class UVerticalBox;

UCLASS()
class AEIDOS_API UPageEquipmentEditorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category="Equipment") void OpenForPage(APageCharacter* InPage);
	UFUNCTION(BlueprintCallable, Category="Equipment") void CloseEditor();

protected:
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_PageName;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_SelectedItem;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Instructions;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UVerticalBox> VerticalBox_InventoryEquipment;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_LeftHand;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_RightHand;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_Head;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_UpperBody;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_LowerBody;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_Feet;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_LeftHand;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_RightHand;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Head;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_UpperBody;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_LowerBody;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Feet;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_Close;

	UPROPERTY(EditDefaultsOnly, Category="Equipment") TSubclassOf<UPageEquipmentItemEntry> EquipmentItemEntryClass;

	UFUNCTION() void HandleLeftHandClicked();
	UFUNCTION() void HandleRightHandClicked();
	UFUNCTION() void HandleHeadClicked();
	UFUNCTION() void HandleUpperBodyClicked();
	UFUNCTION() void HandleLowerBodyClicked();
	UFUNCTION() void HandleFeetClicked();
	UFUNCTION() void HandleItemClicked(FName ItemId);
	UFUNCTION() void HandleEquipmentChanged();
	UFUNCTION() void HandleInventoryChanged();
	UFUNCTION() void HandleCloseClicked();

private:
	TWeakObjectPtr<APageCharacter> EditedPage;
	TWeakObjectPtr<UEquipmentComponent> ObservedEquipment;
	TWeakObjectPtr<UInventoryComponent> ObservedInventory;
	FName SelectedInventoryItem = NAME_None;

	void RefreshEditor();
	void RebuildInventoryEntries();
	void RefreshEquipmentSlots();
	void HandleSlotClicked(EPageEquipmentSlot EquipmentSlot);
	void BindObservedComponents(APageCharacter* Page);
	void UnbindObservedComponents();
	FText GetItemDisplayName(FName ItemId) const;
	FText GetSlotLabel(EPageEquipmentSlot EquipmentSlot) const;
	FText GetCompatibleSlotsText(FName ItemId) const;
	UTextBlock* GetSlotText(EPageEquipmentSlot EquipmentSlot) const;
	UButton* GetSlotButton(EPageEquipmentSlot EquipmentSlot) const;
};
