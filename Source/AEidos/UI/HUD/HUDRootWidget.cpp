// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/HUDRootWidget.h"
#include "UI/HUD/Panels/PanelNavBarWidget.h"
#include "UI/HUD/Panels/PanelHostWidget.h"

void UHUDRootWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (WBP_PanelNavBarWidget)
	{
		WBP_PanelNavBarWidget->OnPanelSelected.AddDynamic(this, &UHUDRootWidget::HandlePanelSelected);
	}
	
	HandlePanelSelected(EInGamePanel::None);
}

void UHUDRootWidget::HandlePanelSelected(EInGamePanel Panel)
{
	const EInGamePanel NextPanel = (ActivePanel == Panel) ? EInGamePanel::None : Panel;
	ActivePanel = NextPanel;

	if (WBP_PanelNavBarWidget)
	{
		WBP_PanelNavBarWidget->SetActivePanel(NextPanel);
	}

	if (WBP_PanelHostWidget)
	{
		WBP_PanelHostWidget->SetPanel(NextPanel);
	}
}
