// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/HorizontalBox.h"
#include "Components/WidgetSwitcher.h"
#include "UI/HUD/Panels/EInGamePanel.h"
#include "PanelHostWidget.generated.h"

/**
 * 
 */
UCLASS()
class AEIDOS_API UPanelHostWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void SetPanel(EInGamePanel NewPanel);

protected:
	UPROPERTY(meta=(BindWidget)) UHorizontalBox* HorizontalBox;
	
	UPROPERTY(meta=(BindWidget))
	UWidgetSwitcher* Switcher_Center;

private:
	EInGamePanel CurrentPanel = EInGamePanel::Recruit;
	int32 PanelToIndex(EInGamePanel Panel) const;

	void CallShown(UWidget* Widget);
	void CallHidden(UWidget* Widget);
};
