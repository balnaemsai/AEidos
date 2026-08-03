// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/HUDRootWidget.h"
#include "UI/HUD/Panels/PanelNavBarWidget.h"
#include "UI/HUD/Panels/PanelHostWidget.h"
#include "UI/HUD/Panels/Panel_Pages.h"
#include "UI/HUD/Panels/PageSkillEditorWidget.h"
#include "Components/OverlaySlot.h"

void UHUDRootWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!PageSkillEditorClass)
	{
		PageSkillEditorClass = LoadClass<UPageSkillEditorWidget>(nullptr,
			TEXT("/Game/Blueprints/WBP/WBP_PageSkillEditor.WBP_PageSkillEditor_C"));
	}

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

bool UHUDRootWidget::CloseActivePanel()
{
	if (ActivePanel == EInGamePanel::None)
	{
		return false;
	}

	ActivePanel = EInGamePanel::None;

	if (WBP_PanelNavBarWidget)
	{
		WBP_PanelNavBarWidget->SetActivePanel(EInGamePanel::None);
	}

	if (WBP_PanelHostWidget)
	{
		WBP_PanelHostWidget->SetPanel(EInGamePanel::None);
	}

	return true;
}

bool UHUDRootWidget::ShowPageSkillEditor(UPanel_Pages* SourcePanel)
{
	if (!SourcePanel || !Layer_Modal || !PageSkillEditorClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Pages] Cannot open skill editor. Source=%s Layer=%s Class=%s"),
			*GetNameSafe(SourcePanel), *GetNameSafe(Layer_Modal), *GetNameSafe(PageSkillEditorClass));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("[Pages] Opening skill editor Class=%s for PageId=%d"),
		*GetNameSafe(PageSkillEditorClass), SourcePanel->GetSelectedPageId());

	if (ActivePageSkillEditor)
	{
		ActivePageSkillEditor->RemoveFromParent();
		ActivePageSkillEditor = nullptr;
	}

	ActivePageSkillEditor = CreateWidget<UPageSkillEditorWidget>(GetOwningPlayer(), PageSkillEditorClass);
	if (!ActivePageSkillEditor)
	{
		return false;
	}

	if (UOverlaySlot* ModalSlot = Layer_Modal->AddChildToOverlay(ActivePageSkillEditor))
	{
		ModalSlot->SetHorizontalAlignment(HAlign_Center);
		ModalSlot->SetVerticalAlignment(VAlign_Center);
	}

	ActivePageSkillEditor->OpenForPanel(SourcePanel);
	return true;
}
