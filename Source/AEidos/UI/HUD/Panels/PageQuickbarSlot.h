#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/HUD/HUDViewModels.h"
#include "PageQuickbarSlot.generated.h"

class UBorder;
class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPageQuickbarSlotClicked, int32, SlotIndex);

UCLASS()
class AEIDOS_API UPageQuickbarSlot : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category="Pages")
	void Setup(const FPageQuickSlotView& InView);

	// The same slot WBP is reused by the editor; only its selection treatment changes.
	UFUNCTION(BlueprintCallable, Category="Pages")
	void SetEditorSelected(bool bInEditorSelected);

	UPROPERTY(BlueprintAssignable, Category="Pages")
	FOnPageQuickbarSlotClicked OnSlotClicked;

protected:
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UButton> Button_Root;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UBorder> Border_Background;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_KeyNumber;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ActionName;

	UPROPERTY(BlueprintReadOnly, Category="Pages")
	FPageQuickSlotView ViewData;

	bool bEditorSelected = false;
	void RefreshPresentation();

	UFUNCTION()
	void HandleClicked();
};
