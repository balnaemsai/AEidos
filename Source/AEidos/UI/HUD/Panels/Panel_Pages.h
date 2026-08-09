// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/HUD/HUDViewModels.h"
#include "Entities/Page/PageCharacter.h"
#include "UI/HUD/Panels/PanelLifeCycle.h"
#include "Panel_Pages.generated.h"

class UPageEntry;
class UPageQuickbarSlot;
class UButton;
class UProgressBar;
class UStatsComponent;
class UTextBlock;
class UUniformGridPanel;
class UVerticalBox;
class UWrapBox;

UCLASS()
class AEIDOS_API UPanel_Pages : public UUserWidget, public IPanelLifecycle
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void OnPanelShown_Implementation() override;
	virtual void OnPanelHidden_Implementation() override;

	UFUNCTION(BlueprintCallable, Category="Pages")
	void RefreshFromWorld();

	UFUNCTION(BlueprintCallable, Category="Pages")
	bool RequestSelectPage(int32 PageId);

	UFUNCTION(BlueprintCallable, Category="Pages")
	bool AssignSelectedPageQuickSlot(int32 SlotIndex, const FPageCombatActionSlot& SlotData);

	UFUNCTION(BlueprintCallable, Category="Pages")
	bool ClearSelectedPageQuickSlot(int32 SlotIndex);

	UFUNCTION(BlueprintPure, Category="Pages")
	const TArray<FPageSummaryView>& GetCachedPages() const { return CachedPages; }

	UFUNCTION(BlueprintPure, Category="Pages")
	const FPageSummaryView& GetSelectedPageSummary() const { return SelectedPageSummary; }

	UFUNCTION(BlueprintPure, Category="Pages")
	int32 GetSelectedPageId() const { return SelectedPageSummary.PageId; }

	UFUNCTION(BlueprintPure, Category="Pages")
	const TArray<FPageQuickSlotView>& GetSelectedPageQuickSlots() const { return SelectedPageQuickSlots; }

	UFUNCTION(BlueprintPure, Category="Pages")
	const TArray<FPageActionCandidateView>& GetSelectedPageAvailableActions() const { return SelectedPageAvailableActions; }

protected:
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UVerticalBox> VerticalBox_PageEntries;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_SelectedPageName;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UProgressBar> ProgressBar_HP;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_HPValue;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Status;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UWrapBox> WrapBox_Quickbar;

	// Fixed action bars must retain their 1-0 order regardless of DPI scale or panel width.
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UUniformGridPanel> UniformGrid_Quickbar;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Volume;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Weight;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UButton> Button_Details;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UButton> Button_EditSkills;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UButton> Button_Equipment;

	// Visible only while a captive entry is being inspected.
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UButton> Button_RecruitCaptive;

	UPROPERTY(EditDefaultsOnly, Category="Pages")
	TSubclassOf<UPageEntry> PageEntryClass;

	// Optional presentation class. A text fallback is used until its WBP is assigned.
	UPROPERTY(EditDefaultsOnly, Category="Pages")
	TSubclassOf<UPageQuickbarSlot> QuickbarSlotClass;

	UPROPERTY(EditDefaultsOnly, Category="Pages", meta=(ClampMin="1", ClampMax="10"))
	int32 QuickbarColumns = 5;

	UPROPERTY(BlueprintReadOnly, Category="Pages")
	TArray<FPageSummaryView> CachedPages;

	UPROPERTY(BlueprintReadOnly, Category="Pages")
	FPageSummaryView SelectedPageSummary;

	UPROPERTY(BlueprintReadOnly, Category="Pages")
	TArray<FPageQuickSlotView> SelectedPageQuickSlots;

	UPROPERTY(BlueprintReadOnly, Category="Pages")
	TArray<FPageActionCandidateView> SelectedPageAvailableActions;

	TWeakObjectPtr<UStatsComponent> ObservedStats;
	TWeakObjectPtr<APageCharacter> InspectedCaptive;
	FDelegateHandle ActorDestroyedHandle;

	void RebuildPageEntryWidgets();
	void RefreshDetailWidgets();
	void RefreshSelectedPageVitals();
	void RebuildQuickbarWidgets();
	void RebindSelectedPageStats(APageCharacter* SelectedPage);
	void UnbindObservedStats();
	void BindWorldActorDestroyed();
	void UnbindWorldActorDestroyed();

	UFUNCTION()
	void HandlePageEntryClicked(int32 PageId);

	UFUNCTION()
	void HandleDetailsClicked();

	UFUNCTION()
	void HandleEditSkillsClicked();

	UFUNCTION()
	void HandleEquipmentClicked();

	UFUNCTION()
	void HandleRecruitCaptiveClicked();

	UFUNCTION()
	void HandleSelectedPageStatsChanged();

	void HandleWorldActorDestroyed(AActor* DestroyedActor);

	UFUNCTION(BlueprintImplementableEvent, Category="Pages")
	void OnDetailsRequested(const FPageSummaryView& Page);

	UFUNCTION(BlueprintImplementableEvent, Category="Pages")
	void OnEditSkillsRequested(const FPageSummaryView& Page);
};
