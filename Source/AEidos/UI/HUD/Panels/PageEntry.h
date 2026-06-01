// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/HUD/HUDViewModels.h"
#include "PageEntry.generated.h"

class UBorder;
class UButton;
class UImage;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPageEntryClicked, int32, PageId);

UCLASS()
class AEIDOS_API UPageEntry : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category="Pages")
	void Setup(const FPageSummaryView& InView);

	UPROPERTY(BlueprintAssignable, Category="Pages")
	FOnPageEntryClicked OnEntryClicked;

protected:
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UBorder> Border_Selection;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_Root;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UImage> Image_Portrait;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UImage> Image_DungeonState;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UImage> Image_PrisonerState;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Name;

	UPROPERTY(BlueprintReadOnly, Category="Pages")
	FPageSummaryView ViewData;

	UFUNCTION()
	void HandleClicked();
};
