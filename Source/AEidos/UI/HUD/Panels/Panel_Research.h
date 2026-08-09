// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/HUD/Panels/PanelLifeCycle.h"
#include "Panel_Research.generated.h"

class UVerticalBox;
class UTextBlock;
class UButton;
class UResearchEntry;
class UWS_Research;
struct FTimerHandle;

UCLASS()
class AEIDOS_API UPanel_Research : public UUserWidget, public IPanelLifecycle
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void OnPanelShown_Implementation() override;
	virtual void OnPanelHidden_Implementation() override;
	UFUNCTION(BlueprintCallable, Category="Research") void RefreshResearchList();

protected:
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UVerticalBox> VerticalBox_ResearchEntries;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Empty;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_Close;
	UPROPERTY(EditDefaultsOnly, Category="Research") TSubclassOf<UResearchEntry> ResearchEntryClass;
	UFUNCTION() void HandleStartResearch(FName ResearchId);
	UFUNCTION() void HandleCancelResearch(FName ResearchId);
	UFUNCTION() void HandleCloseClicked();

private:
	/** Updates text on existing cards so progress refreshes without rebuilding the list. */
	void RefreshResearchProgress();
	void StartProgressRefresh();
	void StopProgressRefresh();

	TWeakObjectPtr<UWS_Research> ObservedResearch;
	FDelegateHandle ResearchChangedHandle;
	TMap<FName, TObjectPtr<UResearchEntry>> ResearchEntriesById;
	FTimerHandle ProgressRefreshTimer;
};
