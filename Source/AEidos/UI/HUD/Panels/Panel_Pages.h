// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/HUD/HUDViewModels.h"
#include "Entities/Page/PageCharacter.h"
#include "UI/HUD/Panels/PanelLifeCycle.h"
#include "Panel_Pages.generated.h"

class UPageEntry;
class UWrapBox;

UCLASS()
class AEIDOS_API UPanel_Pages : public UUserWidget, public IPanelLifecycle
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
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
	const TArray<FPageQuickSlotView>& GetSelectedPageQuickSlots() const { return SelectedPageQuickSlots; }

	UFUNCTION(BlueprintPure, Category="Pages")
	const TArray<FPageActionCandidateView>& GetSelectedPageAvailableActions() const { return SelectedPageAvailableActions; }

protected:
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UWrapBox> WrapBox_PageEntries;

	UPROPERTY(EditDefaultsOnly, Category="Pages")
	TSubclassOf<UPageEntry> PageEntryClass;

	UPROPERTY(BlueprintReadOnly, Category="Pages")
	TArray<FPageSummaryView> CachedPages;

	UPROPERTY(BlueprintReadOnly, Category="Pages")
	FPageSummaryView SelectedPageSummary;

	UPROPERTY(BlueprintReadOnly, Category="Pages")
	TArray<FPageQuickSlotView> SelectedPageQuickSlots;

	UPROPERTY(BlueprintReadOnly, Category="Pages")
	TArray<FPageActionCandidateView> SelectedPageAvailableActions;

	void RebuildPageEntryWidgets();

	UFUNCTION()
	void HandlePageEntryClicked(int32 PageId);
};
