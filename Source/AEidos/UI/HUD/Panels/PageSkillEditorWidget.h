#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/HUD/HUDViewModels.h"
#include "PageSkillEditorWidget.generated.h"

class UButton;
class UPanel_Pages;
class UPageSkillEditorActionEntry;
class UPageQuickbarSlot;
class UTextBlock;
class UUniformGridPanel;
class UVerticalBox;

UCLASS()
class AEIDOS_API UPageSkillEditorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category="Pages")
	void OpenForPanel(UPanel_Pages* InSourcePanel);

	UFUNCTION(BlueprintCallable, Category="Pages")
	void CloseEditor();

protected:
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_PageName;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_SelectedAction;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_SelectedSlot;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UVerticalBox> VerticalBox_AvailableActions;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UUniformGridPanel> UniformGrid_QuickbarSlots;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_ClearSlot;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_Close;

	UPROPERTY(EditDefaultsOnly, Category="Pages") TSubclassOf<UPageSkillEditorActionEntry> ActionEntryClass;
	// Reuses the compact quickbar WBP from the Pages panel, not a second slot asset.
	UPROPERTY(EditDefaultsOnly, Category="Pages") TSubclassOf<UPageQuickbarSlot> QuickbarSlotClass;
	UPROPERTY(EditDefaultsOnly, Category="Pages", meta=(ClampMin="1", ClampMax="10")) int32 QuickbarColumns = 5;

	UPROPERTY(BlueprintReadOnly, Category="Pages") FPageActionCandidateView PendingAction;
	UPROPERTY(BlueprintReadOnly, Category="Pages") int32 SelectedSlotIndex = INDEX_NONE;

	TWeakObjectPtr<UPanel_Pages> SourcePanel;

	void RefreshEditor();
	void RebuildActionEntries();
	void RebuildSlotEntries();
	void RefreshSelectionText();

	UFUNCTION() void HandleActionClicked(FPageActionCandidateView Action);
	UFUNCTION() void HandleSlotClicked(int32 SlotIndex);
	UFUNCTION() void HandleClearSlotClicked();
	UFUNCTION() void HandleCloseClicked();
};
