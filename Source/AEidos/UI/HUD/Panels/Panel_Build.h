// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/Definitions/BuildingDefinitionRow.h"
#include "Panel_Build.generated.h"

class UWrapBox;
class UTextBlock;
class UImage;
class UButton;
class UVerticalBox;
class UBuildEntry;
class UGIS_DataRegistry;
class UDataTable;
struct FWorkDefinitionRow;
struct FWorkCost;

USTRUCT(BlueprintType)
struct FBuildPanelItem
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FName BuildingId;

	UPROPERTY(BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly)
	FText Description;

	UPROPERTY(BlueprintReadOnly)
	EBuildingCategory Category = EBuildingCategory::Production;

	UPROPERTY(BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> ThumbnailIcon;

	UPROPERTY(BlueprintReadOnly)
	FName BuildWorkId;

	UPROPERTY(BlueprintReadOnly)
	TArray<FWorkCost> Costs;

	UPROPERTY(BlueprintReadOnly)
	float TotalWork = 0.f;

	UPROPERTY(BlueprintReadOnly)
	TArray<FName> RequiredResearchIds;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBuildStartRequested, FName, BuildingId);

/**
 * 
 */
UCLASS()
class AEIDOS_API UPanel_Build : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintAssignable, Category="Build")
	FOnBuildStartRequested OnBuildStartRequested;

	UFUNCTION(BlueprintCallable)
	void RefreshBuildList();

	UFUNCTION(BlueprintCallable)
	void SelectBuilding(FName BuildingId);

protected:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UWrapBox> WrapBox_BuildEntries;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> Image_SelectedIcon;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> Text_SelectedName;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> Text_SelectedDesc;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> Text_TotalWorkValue;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> Text_WorkIdValue;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> Text_CategoryValue;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UWrapBox> WrapBox_CostList;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UVerticalBox> VerticalBox_RequirementList;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> Button_StartBuild;
	
	UPROPERTY(EditDefaultsOnly, Category="Build")
	TSubclassOf<UBuildEntry> BuildEntryClass;

	UPROPERTY(BlueprintReadOnly)
	TArray<FBuildPanelItem> CachedItems;

	UPROPERTY(BlueprintReadOnly)
	FName SelectedBuildingId = NAME_None;

protected:
	UFUNCTION()
	void HandleStartBuildClicked();

	void RebuildDetail();
	void ClearChildrenSafe(UPanelWidget* Panel);
	bool BuildItemFromTables(FName BuildingId, FBuildPanelItem& OutItem) const;
	
};
