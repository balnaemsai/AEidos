// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BaseHUDWidget.generated.h"

class UTopBarWidget;
class UResourcePanelWidget;
class UMinimapWidget;
class UWS_Economy;
class UPageRosterWidget;
class UDungeonStatusWidget;
class UNotificationFeedWidget;
class UQuickBarWidget;

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
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	UPROPERTY(meta = (BindWidget))
	UTopBarWidget* TopBar;

	UPROPERTY(meta = (BindWidget))
	UResourcePanelWidget* ResourcePanel;

	UPROPERTY(meta = (BindWidget))
	UMinimapWidget* Minimap;

	UPROPERTY(meta = (BindWidgetOptional))
	UPageRosterWidget* PageRoster;

	UPROPERTY(meta = (BindWidgetOptional))
	UDungeonStatusWidget* DungeonStatus;

	UPROPERTY(meta = (BindWidgetOptional))
	UNotificationFeedWidget* NotificationFeed;

	UPROPERTY(meta = (BindWidgetOptional))
	UQuickBarWidget* QuickBarPopup;

private:
	UPROPERTY()
	UWS_Economy* Economy = nullptr;

	float HUDRefreshAccumulator = 0.f;

	UFUNCTION()
	void HandleEconomyChanged();

	void BindEconomy();
	void UnbindEconomy();
	void RefreshResourcePanel();
	void RefreshPageHUD();
};
