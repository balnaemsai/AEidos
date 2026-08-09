#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PageEquipmentItemEntry.generated.h"

class UBorder;
class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPageEquipmentItemClicked, FName, ItemId);

UCLASS()
class AEIDOS_API UPageEquipmentItemEntry : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	void Setup(FName InItemId, const FText& InDisplayName, int32 InQuantity, const FText& InCompatibleSlots, bool bSelected);

	UPROPERTY(BlueprintAssignable, Category="Equipment")
	FOnPageEquipmentItemClicked OnItemClicked;

protected:
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_Root;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UBorder> Border_Background;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_ItemName;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Quantity;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_CompatibleSlots;

	UFUNCTION() void HandleClicked();

private:
	FName ItemId = NAME_None;
};
