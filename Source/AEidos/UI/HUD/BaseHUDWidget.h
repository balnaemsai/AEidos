// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BaseHUDWidget.generated.h"

class UTopBarWidget;
class UResourcePanelWidget;
class UMinimapWidget;
class UWS_Economy;

/**
 * 
 */
UCLASS()
class AEIDOS_API UBaseHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	UPROPERTY(meta = (BindWidget))
	UTopBarWidget* TopBar;

	UPROPERTY(meta = (BindWidget))
	UResourcePanelWidget* ResourcePanel;

	UPROPERTY(meta = (BindWidget))
	UMinimapWidget* Minimap;

private:
	UPROPERTY()
	UWS_Economy* Economy = nullptr;

	UFUNCTION()
	void HandleEconomyChanged();

	void BindEconomy();
	void UnbindEconomy();
	void RefreshResourcePanel();
};
