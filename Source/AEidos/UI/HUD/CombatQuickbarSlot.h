#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/HUD/HUDViewModels.h"
#include "CombatQuickbarSlot.generated.h"

class UBorder;
class UButton;
class UImage;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatQuickbarSlotClicked, int32, SlotIndex);

UCLASS()
class AEIDOS_API UCombatQuickbarSlot : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	void Setup(const FPageQuickSlotView& InView);

	UPROPERTY(BlueprintAssignable, Category="Combat")
	FOnCombatQuickbarSlotClicked OnSlotClicked;

protected:
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UBorder> Border_SelectionFrame;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UBorder> Border_Surface;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_Action;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Key;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UImage> Image_SkillIcon;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_ActionName;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_APCost;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UImage> Image_DisabledMark;

	UPROPERTY(BlueprintReadOnly, Category="Combat")
	FPageQuickSlotView ViewData;

	FText CachedTooltipText;

	UFUNCTION() void HandleClicked();
};
