// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/HorizontalBox.h"
#include "Components/WidgetSwitcher.h"
#include "UI/HUD/Panels/EInGamePanel.h"
#include "PanelHostWidget.generated.h"

class UPanel_Buildings;

UCLASS()
class AEIDOS_API UPanelHostWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable)
	void SetPanel(EInGamePanel NewPanel);

	UFUNCTION(BlueprintPure)
	EInGamePanel GetCurrentPanel() const { return CurrentPanel; }

protected:
	UPROPERTY(meta=(BindWidget)) UHorizontalBox* HorizontalBox;
	UPROPERTY(meta=(BindWidget)) UWidgetSwitcher* Switcher_Center;

	UFUNCTION()
	void HandleBuildStartRequested(FName BuildingId);

	void BindActiveBuildPanel();

	UPROPERTY(Transient)
	TWeakObjectPtr<UPanel_Buildings> CachedBuildPanel;

private:
	EInGamePanel CurrentPanel = EInGamePanel::None;
	UWidget* FindPanelWidget(EInGamePanel Panel) const;
	void CallShown(UWidget* Widget);
	void CallHidden(UWidget* Widget);
	void RefreshHostVisibility();
};
