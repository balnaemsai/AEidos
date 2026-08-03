#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/Types/PortalTypes.h"
#include "UI/HUD/HUDViewModels.h"
#include "UI/HUD/Panels/PanelLifeCycle.h"
#include "Panel_Dungeons.generated.h"

class UButton;
class UDungeonEntry;
class UTextBlock;
class UVerticalBox;

UCLASS()
class AEIDOS_API UPanel_Dungeons : public UUserWidget, public IPanelLifecycle
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void OnPanelShown_Implementation() override;
	virtual void OnPanelHidden_Implementation() override;

	UFUNCTION(BlueprintCallable, Category="Dungeons")
	void RefreshFromWorld();

	UFUNCTION(BlueprintCallable, Category="Dungeons")
	bool RequestSelectPortal(int32 PortalId);

	UFUNCTION(BlueprintPure, Category="Dungeons")
	const TArray<FPortalState>& GetCachedPortals() const { return CachedPortals; }

	UFUNCTION(BlueprintPure, Category="Dungeons")
	const TArray<FDungeonPortalView>& GetCachedPortalViews() const { return CachedPortalViews; }

	UFUNCTION(BlueprintPure, Category="Dungeons")
	const FDungeonPortalView& GetSelectedPortalView() const { return SelectedPortalView; }

	UFUNCTION(BlueprintPure, Category="Dungeons")
	bool IsSelectedPageInDungeon() const { return bSelectedPageInDungeon; }

protected:
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UVerticalBox> VerticalBox_PortalEntries;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_SelectedPortalName;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_SelectedTier;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_PortalStatus;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_RaidTimer;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ExpeditionStatus;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_PortalDistance;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_EmptyState;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UButton> Button_FocusPortal;

	UPROPERTY(EditDefaultsOnly, Category="Dungeons")
	TSubclassOf<UDungeonEntry> DungeonEntryClass;

	UPROPERTY(EditDefaultsOnly, Category="Dungeons", meta=(ClampMin="0.1"))
	float RefreshIntervalSeconds = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category="Dungeons")
	TArray<FPortalState> CachedPortals;

	UPROPERTY(BlueprintReadOnly, Category="Dungeons")
	TArray<FDungeonPortalView> CachedPortalViews;

	UPROPERTY(BlueprintReadOnly, Category="Dungeons")
	FDungeonPortalView SelectedPortalView;

	UPROPERTY(BlueprintReadOnly, Category="Dungeons")
	bool bSelectedPageInDungeon = false;

	UPROPERTY(BlueprintReadOnly, Category="Dungeons")
	int32 SelectedPortalId = INDEX_NONE;

	void RebuildPortalEntryWidgets();
	void RefreshDetailWidgets();
	void StartAutoRefresh();
	void StopAutoRefresh();

	UFUNCTION()
	void HandleDungeonEntryClicked(int32 PortalId);

	UFUNCTION()
	void HandleFocusPortalClicked();

	UFUNCTION(BlueprintImplementableEvent, Category="Dungeons")
	void OnFocusPortalRequested(int32 PortalId);

private:
	FTimerHandle RefreshTimerHandle;
	bool bHasLoggedEntrySetupFailure = false;
};
