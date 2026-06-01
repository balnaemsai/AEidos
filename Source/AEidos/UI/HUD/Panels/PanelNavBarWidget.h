// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "UI/HUD/Panels/EInGamePanel.h"
#include "Styling/SlateTypes.h"
#include "PanelNavBarWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPanelSelected, EInGamePanel, Panel);

UCLASS()
class AEIDOS_API UPanelNavBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnPanelSelected OnPanelSelected;

	UFUNCTION(BlueprintCallable)
	void SetActivePanel(EInGamePanel Panel);

	UFUNCTION(BlueprintPure)
	EInGamePanel GetActivePanel() const { return ActivePanel; }

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta=(BindWidget)) UButton* BtnBuildings;
	UPROPERTY(meta=(BindWidget)) UButton* BtnPages;
	UPROPERTY(meta=(BindWidget)) UButton* BtnDungeons;
	UPROPERTY(meta=(BindWidget)) UButton* BtnItems;
	UPROPERTY(meta=(BindWidget)) UButton* BtnResearch;

	UPROPERTY(EditDefaultsOnly, Category="Style")
	FButtonStyle ActiveStyle;

	UPROPERTY(EditDefaultsOnly, Category="Style")
	FButtonStyle InactiveStyle;

	UPROPERTY(EditDefaultsOnly, Category="Style")
	float ActiveBrightnessMul = 1.20f;

	UPROPERTY(EditDefaultsOnly, Category="Style")
	float ActiveSaturationMul = 1.15f;

private:
	bool bStylesInitialized = false;
	EInGamePanel ActivePanel = EInGamePanel::None;

	void InitStylesFrom(UButton* AnyButton);
	static FSlateBrush BrightenBrush(const FSlateBrush& InBrush, float BrightMul, float SatMul);
	void Apply(UButton* Btn, bool bActive);
	void Refresh(EInGamePanel Panel);

	UFUNCTION() void HandleBuildings();
	UFUNCTION() void HandlePages();
	UFUNCTION() void HandleDungeons();
	UFUNCTION() void HandleItems();
	UFUNCTION() void HandleResearch();
};
