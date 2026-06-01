// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/HUD/Panels/PanelHostWidget.h"
#include "UI/HUD/Panels/PanelLifeCycle.h"
#include "UI/HUD/Panels/Panel_Buildings.h"
#include "UI/HUD/Panels/Panel_Dungeons.h"
#include "UI/HUD/Panels/Panel_Items.h"
#include "UI/HUD/Panels/Panel_Pages.h"
#include "UI/HUD/Panels/Panel_Research.h"
#include "FrameWork/EidosPlayerController.h"

namespace
{
	const FName PanelHost_TerritoryExpansionBuildId(TEXT("TerritoryExpansion"));
}

void UPanelHostWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshHostVisibility();
	BindActiveBuildPanel();
}

void UPanelHostWidget::HandleBuildStartRequested(FName BuildingId)
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (AEidosPlayerController* EidosPC = Cast<AEidosPlayerController>(PC))
		{
			if (BuildingId == PanelHost_TerritoryExpansionBuildId)
			{
				EidosPC->BeginTerritoryExpansionPlacement();
			}
			else
			{
				EidosPC->BeginBuildPlacement(BuildingId);
			}
		}
	}
}

UWidget* UPanelHostWidget::FindPanelWidget(EInGamePanel Panel) const
{
	if (!Switcher_Center)
	{
		return nullptr;
	}

	for (int32 Index = 0; Index < Switcher_Center->GetNumWidgets(); ++Index)
	{
		UWidget* Child = Switcher_Center->GetWidgetAtIndex(Index);
		if (!Child)
		{
			continue;
		}

		switch (Panel)
		{
		case EInGamePanel::Buildings:
			if (Child->IsA<UPanel_Buildings>())
			{
				return Child;
			}
			break;
		case EInGamePanel::Pages:
			if (Child->IsA<UPanel_Pages>())
			{
				return Child;
			}
			break;
		case EInGamePanel::Dungeons:
			if (Child->IsA<UPanel_Dungeons>())
			{
				return Child;
			}
			break;
		case EInGamePanel::Items:
			if (Child->IsA<UPanel_Items>())
			{
				return Child;
			}
			break;
		case EInGamePanel::Research:
			if (Child->IsA<UPanel_Research>())
			{
				return Child;
			}
			break;
		default:
			break;
		}
	}

	return nullptr;
}

void UPanelHostWidget::CallShown(UWidget* Widget)
{
	if (Widget && Widget->GetClass()->ImplementsInterface(UPanelLifecycle::StaticClass()))
	{
		IPanelLifecycle::Execute_OnPanelShown(Widget);
	}
}

void UPanelHostWidget::CallHidden(UWidget* Widget)
{
	if (Widget && Widget->GetClass()->ImplementsInterface(UPanelLifecycle::StaticClass()))
	{
		IPanelLifecycle::Execute_OnPanelHidden(Widget);
	}
}

void UPanelHostWidget::SetPanel(EInGamePanel NewPanel)
{
	if (!Switcher_Center)
	{
		return;
	}

	if (NewPanel == CurrentPanel)
	{
		RefreshHostVisibility();
		return;
	}

	if (CurrentPanel != EInGamePanel::None)
	{
		CallHidden(Switcher_Center->GetActiveWidget());
	}

	CurrentPanel = NewPanel;

	if (CurrentPanel != EInGamePanel::None)
	{
		if (UWidget* TargetWidget = FindPanelWidget(CurrentPanel))
		{
			Switcher_Center->SetActiveWidget(TargetWidget);
			CallShown(TargetWidget);
		}
	}

	RefreshHostVisibility();
	BindActiveBuildPanel();
}

void UPanelHostWidget::BindActiveBuildPanel()
{
	if (!Switcher_Center || CurrentPanel == EInGamePanel::None)
	{
		return;
	}

	UWidget* ActiveWidget = Switcher_Center->GetActiveWidget();
	UPanel_Buildings* Build = Cast<UPanel_Buildings>(ActiveWidget);
	if (!Build)
	{
		return;
	}

	if (CachedBuildPanel.Get() == Build)
	{
		return;
	}

	Build->OnBuildStartRequested.RemoveDynamic(this, &UPanelHostWidget::HandleBuildStartRequested);
	Build->OnBuildStartRequested.AddDynamic(this, &UPanelHostWidget::HandleBuildStartRequested);
	CachedBuildPanel = Build;
}

void UPanelHostWidget::RefreshHostVisibility()
{
	SetVisibility(CurrentPanel == EInGamePanel::None ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
}
