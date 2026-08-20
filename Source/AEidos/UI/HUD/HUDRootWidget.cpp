// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/HUDRootWidget.h"
#include "UI/HUD/Panels/PanelNavBarWidget.h"
#include "UI/HUD/Panels/PanelHostWidget.h"
#include "UI/HUD/Panels/Panel_Pages.h"
#include "UI/HUD/Panels/PageSkillEditorWidget.h"
#include "UI/HUD/Panels/PageEquipmentEditorWidget.h"
#include "UI/HUD/Panels/PageWorkPriorityEditorWidget.h"
#include "UI/HUD/Panels/WorkOrderPopupWidget.h"
#include "UI/HUD/SettlementDefeatWidget.h"
#include "UI/HUD/ScenarioVictoryWidget.h"
#include "Components/OverlaySlot.h"
#include "Framework/EidosGameMode.h"
#include "GameFramework/PlayerController.h"

void UHUDRootWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!PageSkillEditorClass)
	{
		PageSkillEditorClass = LoadClass<UPageSkillEditorWidget>(nullptr,
			TEXT("/Game/Blueprints/WBP/WBP_PageSkillEditor.WBP_PageSkillEditor_C"));
	}
	if (!PageEquipmentEditorClass)
	{
		PageEquipmentEditorClass = LoadClass<UPageEquipmentEditorWidget>(nullptr,
			TEXT("/Game/Blueprints/WBP/WBP_PageEquipmentEditor.WBP_PageEquipmentEditor_C"));
	}
	if (!WorkOrderPopupClass)
	{
		WorkOrderPopupClass = LoadClass<UWorkOrderPopupWidget>(nullptr, TEXT("/Game/Blueprints/WBP/WBP_WorkOrderPopup.WBP_WorkOrderPopup_C"));
	}
	if (!PageWorkPriorityEditorClass)
	{
		PageWorkPriorityEditorClass = LoadClass<UPageWorkPriorityEditorWidget>(nullptr,
			TEXT("/Game/Blueprints/WBP/WBP_PageWorkPriorityEditor.WBP_PageWorkPriorityEditor_C"));
	}
	if (!SettlementDefeatWidgetClass)
	{
		SettlementDefeatWidgetClass = LoadClass<USettlementDefeatWidget>(nullptr,
			TEXT("/Game/Blueprints/WBP/WBP_SettlementDefeat.WBP_SettlementDefeat_C"));
	}
	if (!ScenarioVictoryWidgetClass)
	{
		ScenarioVictoryWidgetClass = LoadClass<UScenarioVictoryWidget>(nullptr,
			TEXT("/Game/Blueprints/WBP/WBP_ScenarioVictory.WBP_ScenarioVictory_C"));
	}

	if (WBP_PanelNavBarWidget)
	{
		WBP_PanelNavBarWidget->OnPanelSelected.AddDynamic(this, &UHUDRootWidget::HandlePanelSelected);
	}
	
	HandlePanelSelected(EInGamePanel::None);

	if (AEidosGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEidosGameMode>() : nullptr)
	{
		GameMode->OnGameOver.AddUniqueDynamic(this, &UHUDRootWidget::HandleGameOver);
		GameMode->OnGameVictory.AddUniqueDynamic(this, &UHUDRootWidget::HandleGameVictory);
		if (GameMode->IsGameOver())
		{
			ShowSettlementDefeat();
		}
		else if (GameMode->IsGameVictory())
		{
			ShowScenarioVictory(GameMode->GetVictoryScenarioName(), GameMode->GetVictoryScenarioDescription());
		}
	}
}

void UHUDRootWidget::NativeDestruct()
{
	if (AEidosGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEidosGameMode>() : nullptr)
	{
		GameMode->OnGameOver.RemoveDynamic(this, &UHUDRootWidget::HandleGameOver);
		GameMode->OnGameVictory.RemoveDynamic(this, &UHUDRootWidget::HandleGameVictory);
	}

	Super::NativeDestruct();
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

bool UHUDRootWidget::ShowPageEquipmentEditor(APageCharacter* Page)
{
	if (!Page || !Layer_Modal || !PageEquipmentEditorClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Equipment] Cannot open editor. Page=%s Layer=%s Class=%s"),
			*GetNameSafe(Page), *GetNameSafe(Layer_Modal), *GetNameSafe(PageEquipmentEditorClass));
		return false;
	}

	if (ActivePageEquipmentEditor)
	{
		ActivePageEquipmentEditor->RemoveFromParent();
		ActivePageEquipmentEditor = nullptr;
	}

	ActivePageEquipmentEditor = CreateWidget<UPageEquipmentEditorWidget>(GetOwningPlayer(), PageEquipmentEditorClass);
	if (!ActivePageEquipmentEditor) return false;
	if (UOverlaySlot* ModalSlot = Layer_Modal->AddChildToOverlay(ActivePageEquipmentEditor))
	{
		ModalSlot->SetHorizontalAlignment(HAlign_Center);
		ModalSlot->SetVerticalAlignment(VAlign_Center);
	}
	ActivePageEquipmentEditor->OpenForPage(Page);
	return true;
}

bool UHUDRootWidget::ShowPageWorkPriorityEditor(APageCharacter* Page)
{
	if (!Page || !Layer_Modal || !PageWorkPriorityEditorClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Pages] Cannot open work priority editor. Page=%s Layer=%s Class=%s"),
			*GetNameSafe(Page), *GetNameSafe(Layer_Modal), *GetNameSafe(PageWorkPriorityEditorClass));
		return false;
	}

	if (ActivePageWorkPriorityEditor)
	{
		ActivePageWorkPriorityEditor->RemoveFromParent();
		ActivePageWorkPriorityEditor = nullptr;
	}

	ActivePageWorkPriorityEditor = CreateWidget<UPageWorkPriorityEditorWidget>(GetOwningPlayer(), PageWorkPriorityEditorClass);
	if (!ActivePageWorkPriorityEditor) return false;
	if (UOverlaySlot* ModalSlot = Layer_Modal->AddChildToOverlay(ActivePageWorkPriorityEditor))
	{
		ModalSlot->SetHorizontalAlignment(HAlign_Center);
		ModalSlot->SetVerticalAlignment(VAlign_Center);
	}
	ActivePageWorkPriorityEditor->OpenForPage(Page);
	return true;
}

bool UHUDRootWidget::ShowWorkOrderPopup()
{
	if (!Layer_Modal || !WorkOrderPopupClass) return false;
	if (ActiveWorkOrderPopup) { ActiveWorkOrderPopup->RemoveFromParent(); ActiveWorkOrderPopup = nullptr; }
	ActiveWorkOrderPopup = CreateWidget<UWorkOrderPopupWidget>(GetOwningPlayer(), WorkOrderPopupClass);
	if (!ActiveWorkOrderPopup) return false;
	if (UOverlaySlot* ModalSlot = Layer_Modal->AddChildToOverlay(ActiveWorkOrderPopup)) { ModalSlot->SetHorizontalAlignment(HAlign_Center); ModalSlot->SetVerticalAlignment(VAlign_Center); }
	return true;
}

void UHUDRootWidget::HandleGameOver()
{
	ShowSettlementDefeat();
}

void UHUDRootWidget::HandleGameVictory(FText ScenarioName, FText ScenarioDescription)
{
	ShowScenarioVictory(ScenarioName, ScenarioDescription);
}

void UHUDRootWidget::ShowScenarioVictory(const FText& ScenarioName, const FText& ScenarioDescription)
{
	if (!Layer_Modal || !ScenarioVictoryWidgetClass || ActiveScenarioVictoryWidget)
	{
		return;
	}

	ActiveScenarioVictoryWidget = CreateWidget<UScenarioVictoryWidget>(GetOwningPlayer(), ScenarioVictoryWidgetClass);
	if (!ActiveScenarioVictoryWidget)
	{
		return;
	}

	ActiveScenarioVictoryWidget->SetScenarioResult(ScenarioName, ScenarioDescription);
	if (UOverlaySlot* ModalSlot = Layer_Modal->AddChildToOverlay(ActiveScenarioVictoryWidget))
	{
		ModalSlot->SetHorizontalAlignment(HAlign_Fill);
		ModalSlot->SetVerticalAlignment(VAlign_Fill);
	}

	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(ActiveScenarioVictoryWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PlayerController->SetInputMode(InputMode);
		PlayerController->bShowMouseCursor = true;
	}
}

void UHUDRootWidget::ShowSettlementDefeat()
{
	if (!Layer_Modal || !SettlementDefeatWidgetClass || ActiveSettlementDefeatWidget)
	{
		return;
	}

	ActiveSettlementDefeatWidget = CreateWidget<USettlementDefeatWidget>(GetOwningPlayer(), SettlementDefeatWidgetClass);
	if (!ActiveSettlementDefeatWidget)
	{
		return;
	}

	if (UOverlaySlot* ModalSlot = Layer_Modal->AddChildToOverlay(ActiveSettlementDefeatWidget))
	{
		ModalSlot->SetHorizontalAlignment(HAlign_Fill);
		ModalSlot->SetVerticalAlignment(VAlign_Fill);
	}

	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(ActiveSettlementDefeatWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PlayerController->SetInputMode(InputMode);
		PlayerController->bShowMouseCursor = true;
	}
}
