// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Overlay.h"
#include "Components/CanvasPanel.h"
#include "UI/HUD/Panels/EInGamePanel.h"
#include "HUDRootWidget.generated.h"

class UPanelNavBarWidget;
class UPanelHostWidget;
class UCombatHUDWidget;
class USettlementCoreHUDWidget;
class USettlementDefeatWidget;
class UScenarioVictoryWidget;
class UPanel_Pages;
class UPageSkillEditorWidget;
class UPageEquipmentEditorWidget;
class UPageWorkPriorityEditorWidget;
class UWorkOrderPopupWidget;
class APageCharacter;

/**
 * 
 */
UCLASS()
class AEIDOS_API UHUDRootWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UOverlay* GetBaseHUDLayer() const { return Layer_BaseHUD; }
	UCanvasPanel* GetPagesLayer()   const { return Layer_Panels; }
	UOverlay* GetContextLayer() const { return Layer_Context; }
	UOverlay* GetModalLayer()   const { return Layer_Modal; }

	// Shared exit point for a close button placed inside any in-game panel.
	UFUNCTION(BlueprintCallable, Category="Panel")
	bool CloseActivePanel();

	UFUNCTION(BlueprintCallable, Category="Pages")
	bool ShowPageSkillEditor(UPanel_Pages* SourcePanel);

	UFUNCTION(BlueprintCallable, Category="Equipment")
	bool ShowPageEquipmentEditor(APageCharacter* Page);

	UFUNCTION(BlueprintCallable, Category="Pages|Work")
	bool ShowPageWorkPriorityEditor(APageCharacter* Page);

	UFUNCTION(BlueprintCallable, Category="Work")
	bool ShowWorkOrderPopup();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	UOverlay* Layer_BaseHUD;

	UPROPERTY(meta=(BindWidget))
	UCanvasPanel* Layer_Panels;

	UPROPERTY(meta=(BindWidget)) UPanelHostWidget* WBP_PanelHostWidget;
	UPROPERTY(meta=(BindWidget)) UPanelNavBarWidget* WBP_PanelNavBarWidget;

	UFUNCTION()
	void HandlePanelSelected(EInGamePanel Panel);

	UFUNCTION()
	void HandleGameOver();
	UFUNCTION()
	void HandleGameVictory(FText ScenarioName, FText ScenarioDescription);

	void ShowSettlementDefeat();
	void ShowScenarioVictory(const FText& ScenarioName, const FText& ScenarioDescription);

	EInGamePanel ActivePanel = EInGamePanel::None;

	UPROPERTY(meta = (BindWidget))
	UOverlay* Layer_Context;

	UPROPERTY(meta = (BindWidgetOptional))
	UCombatHUDWidget* CombatHUD;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USettlementCoreHUDWidget> SettlementCoreHUD;

	UPROPERTY(meta = (BindWidget))
	UOverlay* Layer_Modal;

	UPROPERTY(EditDefaultsOnly, Category="Pages")
	TSubclassOf<UPageSkillEditorWidget> PageSkillEditorClass;

	UPROPERTY(Transient)
	TObjectPtr<UPageSkillEditorWidget> ActivePageSkillEditor;

	UPROPERTY(EditDefaultsOnly, Category="Equipment")
	TSubclassOf<UPageEquipmentEditorWidget> PageEquipmentEditorClass;

	UPROPERTY(Transient)
	TObjectPtr<UPageEquipmentEditorWidget> ActivePageEquipmentEditor;

	UPROPERTY(EditDefaultsOnly, Category="Pages|Work")
	TSubclassOf<UPageWorkPriorityEditorWidget> PageWorkPriorityEditorClass;

	UPROPERTY(Transient)
	TObjectPtr<UPageWorkPriorityEditorWidget> ActivePageWorkPriorityEditor;

	UPROPERTY(EditDefaultsOnly, Category="Work")
	TSubclassOf<UWorkOrderPopupWidget> WorkOrderPopupClass;
	UPROPERTY(Transient) TObjectPtr<UWorkOrderPopupWidget> ActiveWorkOrderPopup;

	UPROPERTY(EditDefaultsOnly, Category="Defeat")
	TSubclassOf<USettlementDefeatWidget> SettlementDefeatWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<USettlementDefeatWidget> ActiveSettlementDefeatWidget;

	UPROPERTY(EditDefaultsOnly, Category="Victory")
	TSubclassOf<UScenarioVictoryWidget> ScenarioVictoryWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UScenarioVictoryWidget> ActiveScenarioVictoryWidget;
	
};
