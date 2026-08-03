#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/HUD/HUDViewModels.h"
#include "CombatHUDWidget.generated.h"

class APageCharacter;
class UButton;
class UCombatQuickbarSlot;
class UProgressBar;
class UTextBlock;
class UWrapBox;
class USizeBox;

UCLASS()
class AEIDOS_API UCombatHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<USizeBox> SizeBox_TargetReadout;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_TargetType;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_TargetName;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UProgressBar> ProgressBar_TargetHP;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_TargetHP;

	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<USizeBox> SizeBox_TurnReadout;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_TurnState;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_ActivePageName;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_AP;

	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<USizeBox> SizeBox_CombatCommandBar;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_TargetHint;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_APLarge;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UWrapBox> WrapBox_QuickbarSlots;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_EndTurn;

	UPROPERTY(EditDefaultsOnly, Category="Combat")
	TSubclassOf<UCombatQuickbarSlot> CombatQuickbarSlotClass;

	UPROPERTY(EditDefaultsOnly, Category="Combat", meta=(ClampMin="0.03"))
	float RefreshIntervalSeconds = 0.10f;

private:
	void RefreshFromCombat();
	void RebuildQuickbar(APageCharacter* ActivePage);
	FPageQuickSlotView BuildSlotView(APageCharacter* ActivePage, int32 SlotIndex) const;
	void RefreshTargetReadout(AActor* TargetActor);

	UFUNCTION() void HandleEndTurnClicked();
	UFUNCTION() void HandleSlotClicked(int32 SlotIndex);

	TWeakObjectPtr<APageCharacter> CachedActivePage;
	TArray<TObjectPtr<UCombatQuickbarSlot>> QuickbarSlotWidgets;
	float RefreshAccumulator = 0.f;
};
