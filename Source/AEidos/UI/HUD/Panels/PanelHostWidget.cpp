// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/Panels/PanelHostWidget.h"
#include "UI/HUD/Panels/PanelLifeCycle.h"

int32 UPanelHostWidget::PanelToIndex(EInGamePanel Panel) const
{
	switch (Panel)
	{
	case EInGamePanel::Recruit:   return 0;
	case EInGamePanel::Craft:     return 1;
	case EInGamePanel::Research:  return 2;
	case EInGamePanel::Build:     return 3;
	case EInGamePanel::Buildings: return 4;
	case EInGamePanel::Dungeons:  return 5;
	case EInGamePanel::Pages:     return 6;
	case EInGamePanel::Items:     return 7;
	case EInGamePanel::Relations: return 8;
	case EInGamePanel::Skill:     return 9;
	}
	return 0;
}

void UPanelHostWidget::CallShown(UWidget* Widget)
{
	if (!Widget) return;

	if (Widget->GetClass()->ImplementsInterface(UPanelLifecycle::StaticClass()))
	{
		IPanelLifecycle::Execute_OnPanelShown(Widget);
	}
}

void UPanelHostWidget::CallHidden(UWidget* Widget)
{
	if (!Widget) return;

	if (Widget->GetClass()->ImplementsInterface(UPanelLifecycle::StaticClass()))
	{
		IPanelLifecycle::Execute_OnPanelHidden(Widget);
	}
}

void UPanelHostWidget::SetPanel(EInGamePanel NewPanel)
{
	if (!Switcher_Center || NewPanel == CurrentPanel)
	{
		return;
	}
	
	CallHidden(Switcher_Center->GetActiveWidget());
	
	CurrentPanel = NewPanel;
	Switcher_Center->SetActiveWidgetIndex(PanelToIndex(NewPanel));
	
	CallShown(Switcher_Center->GetActiveWidget());
}