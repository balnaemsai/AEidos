// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Panel_Build.h"
#include "BuildEntry.generated.h"

class UButton;
class UImage;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBuildEntryClicked, FName, BuildingId);

/**
 * 
 */
UCLASS()
class AEIDOS_API UBuildEntry : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable)
	void Setup(const FBuildPanelItem& InItem);

	UPROPERTY(BlueprintAssignable)
	FOnBuildEntryClicked OnEntryClicked;

protected:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> Button_Root;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> Image_Icon;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> Text_Name;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> Text_Category;

	UPROPERTY(BlueprintReadOnly)
	FBuildPanelItem ItemData;

protected:
	UFUNCTION()
	void HandleClicked();
};
