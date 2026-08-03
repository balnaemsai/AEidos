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
class UPanel_Pages;
class UPageSkillEditorWidget;

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

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	UOverlay* Layer_BaseHUD;

	UPROPERTY(meta=(BindWidget))
	UCanvasPanel* Layer_Panels;

	UPROPERTY(meta=(BindWidget)) UPanelHostWidget* WBP_PanelHostWidget;
	UPROPERTY(meta=(BindWidget)) UPanelNavBarWidget* WBP_PanelNavBarWidget;

	UFUNCTION()
	void HandlePanelSelected(EInGamePanel Panel);

	EInGamePanel ActivePanel = EInGamePanel::None;

	UPROPERTY(meta = (BindWidget))
	UOverlay* Layer_Context;

	UPROPERTY(meta = (BindWidgetOptional))
	UCombatHUDWidget* CombatHUD;

	UPROPERTY(meta = (BindWidget))
	UOverlay* Layer_Modal;

	UPROPERTY(EditDefaultsOnly, Category="Pages")
	TSubclassOf<UPageSkillEditorWidget> PageSkillEditorClass;

	UPROPERTY(Transient)
	TObjectPtr<UPageSkillEditorWidget> ActivePageSkillEditor;
	
};
