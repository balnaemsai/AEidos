#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemTransferEntry.generated.h"

class UBorder;
class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnItemTransferEntrySelected, FName, ItemId, bool, bFromStorage, bool, bAdditive);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemTransferEntryContextRequested, FName, ItemId, bool, bFromStorage);

UCLASS()
class AEIDOS_API UItemTransferEntry : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	void Setup(FName InItemId, const FText& InDisplayName, int32 InQuantity, bool bInFromStorage, bool bSelected);

	UPROPERTY(BlueprintAssignable, Category="Items") FOnItemTransferEntrySelected OnEntrySelected;
	UPROPERTY(BlueprintAssignable, Category="Items") FOnItemTransferEntryContextRequested OnEntryContextRequested;

protected:
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_Root;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UBorder> Border_Background;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_ItemName;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Quantity;

private:
	FName ItemId = NAME_None;
	bool bFromStorage = false;
};
