// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/Types/ItemTypes.h"
#include "UI/HUD/Panels/PanelLifeCycle.h"
#include "Panel_Items.generated.h"

class UTextBlock;
class UVerticalBox;

USTRUCT(BlueprintType)
struct FStoredResourceView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FName ResourceId = NAME_None;

	UPROPERTY(BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly)
	int32 Amount = 0;
};

UCLASS()
class AEIDOS_API UPanel_Items : public UUserWidget, public IPanelLifecycle
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void OnPanelShown_Implementation() override;
	virtual void OnPanelHidden_Implementation() override;

	UFUNCTION(BlueprintCallable, Category="Items")
	void RefreshFromWorld();

	UFUNCTION(BlueprintPure, Category="Items")
	const TArray<FStoredResourceView>& GetResourceViews() const { return CachedResourceViews; }

	UFUNCTION(BlueprintPure, Category="Items")
	const TArray<FItemStack>& GetStoredItems() const { return CachedStoredItems; }

	UFUNCTION(BlueprintPure, Category="Items")
	const TArray<FItemStack>& GetSelectedPageItems() const { return CachedSelectedPageItems; }

protected:
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UVerticalBox> VerticalBox_ResourceEntries;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UVerticalBox> VerticalBox_StoredItemEntries;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UVerticalBox> VerticalBox_PageItemEntries;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_StorageWeight;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_StorageVolume;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_SelectedPageInventory;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_EmptyStoredItems;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_EmptyPageItems;

	UPROPERTY(EditDefaultsOnly, Category="Items", meta=(ClampMin="0.1"))
	float RefreshIntervalSeconds = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category="Items")
	TArray<FStoredResourceView> CachedResourceViews;

	UPROPERTY(BlueprintReadOnly, Category="Items")
	TArray<FItemStack> CachedStoredItems;

	UPROPERTY(BlueprintReadOnly, Category="Items")
	TArray<FItemStack> CachedSelectedPageItems;

private:
	void StartAutoRefresh();
	void StopAutoRefresh();
	void RebuildListWidgets();
	FTimerHandle RefreshTimerHandle;
};
