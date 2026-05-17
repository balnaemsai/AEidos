// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/HUD/HUDViewModels.h"
#include "Entities/Page/PageCharacter.h"
#include "Panel_Pages.generated.h"

/**
 * 
 */
UCLASS()
class AEIDOS_API UPanel_Pages : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category="Pages")
	void RefreshFromWorld();

	UFUNCTION(BlueprintCallable, Category="Pages")
	bool RequestSelectPage(int32 PageId);

	UFUNCTION(BlueprintCallable, Category="Pages")
	bool AssignSelectedPageQuickSlot(int32 SlotIndex, const FPageCombatActionSlot& SlotData);

	UFUNCTION(BlueprintPure, Category="Pages")
	const TArray<FPageSummaryView>& GetCachedPages() const { return CachedPages; }

	UFUNCTION(BlueprintPure, Category="Pages")
	const FPageSummaryView& GetSelectedPageSummary() const { return SelectedPageSummary; }

	UFUNCTION(BlueprintPure, Category="Pages")
	const TArray<FPageQuickSlotView>& GetSelectedPageQuickSlots() const { return SelectedPageQuickSlots; }

protected:
	UPROPERTY(BlueprintReadOnly, Category="Pages")
	TArray<FPageSummaryView> CachedPages;

	UPROPERTY(BlueprintReadOnly, Category="Pages")
	FPageSummaryView SelectedPageSummary;

	UPROPERTY(BlueprintReadOnly, Category="Pages")
	TArray<FPageQuickSlotView> SelectedPageQuickSlots;
};
