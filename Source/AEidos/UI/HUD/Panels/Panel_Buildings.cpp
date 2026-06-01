// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/HUD/Panels/Panel_Buildings.h"
#include "UI/HUD/Panels/BuildEntry.h"
#include "Data/GIS_DataRegistry.h"
#include "Data/Definitions/WorkDefinitionRow.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/WrapBox.h"
#include "Components/VerticalBox.h"
#include "Components/WrapBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "World/Settlement/WS_SettlementSpace.h"

namespace
{
	const FName PanelBuild_TerritoryExpansionBuildId(TEXT("TerritoryExpansion"));
}

void UPanel_Buildings::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_StartBuild)
	{
		Button_StartBuild->OnClicked.AddDynamic(this, &UPanel_Buildings::HandleStartBuildClicked);
	}

	RefreshBuildList();
}

void UPanel_Buildings::OnPanelShown_Implementation()
{
	RefreshBuildList();
}

void UPanel_Buildings::OnPanelHidden_Implementation()
{
}

void UPanel_Buildings::RefreshBuildList()
{
	UE_LOG(LogTemp, Warning, TEXT("[BuildPanel] RefreshBuildList START"));
	CachedItems.Reset();

	if (!WrapBox_BuildEntries)
	{
		UE_LOG(LogTemp, Error, TEXT("[BuildPanel] WrapBox_BuildEntries is null"));
		return;
	}

	WrapBox_BuildEntries->ClearChildren();

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("[BuildPanel] GetWorld() is null"));
		return;
	}

	UGameInstance* GI = World->GetGameInstance();
	if (!GI)
	{
		UE_LOG(LogTemp, Error, TEXT("[BuildPanel] GameInstance is null"));
		return;
	}

	UGIS_DataRegistry* Registry = GI->GetSubsystem<UGIS_DataRegistry>();
	if (!Registry)
	{
		UE_LOG(LogTemp, Error, TEXT("[BuildPanel] DataRegistry subsystem is null"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[BuildPanel] DataRegistry valid. Ready=%d Loading=%d Reason=%s"), Registry->IsReady(), Registry->IsLoading(), *Registry->GetNotReadyReason());

	UDataTable* DT = Registry->GetBuildingTable();
	if (!DT)
	{
		UE_LOG(LogTemp, Error, TEXT("[BuildPanel] DT_Building is null"));
		return;
	}

	TArray<FBuildingDefinitionRow*> Rows;
	DT->GetAllRows(TEXT("UPanel_Buildings::RefreshBuildList"), Rows);
	UE_LOG(LogTemp, Warning, TEXT("[BuildPanel] DT_Building row count = %d"), Rows.Num());

	{
		FBuildPanelItem TerritoryItem;
		TerritoryItem.BuildingId = PanelBuild_TerritoryExpansionBuildId;
		TerritoryItem.DisplayName = FText::FromString(TEXT("Territory Expansion"));
		TerritoryItem.Description = FText::FromString(TEXT("Claim an adjacent chunk immediately after placement confirmation. This skips worker construction, but still spends the territory cost."));
		TerritoryItem.Category = EBuildingCategory::Structure;
		TerritoryItem.BuildWorkId = NAME_None;
		TerritoryItem.TotalWork = 0.f;
		if (UWS_SettlementSpace* SettlementSpace = World->GetSubsystem<UWS_SettlementSpace>())
		{
			if (!SettlementSpace->GetExpansionCostResourceId().IsNone() && SettlementSpace->GetExpansionCostAmount() > 0)
			{
				FWorkCost Cost;
				Cost.ResourceId = SettlementSpace->GetExpansionCostResourceId();
				Cost.Amount = SettlementSpace->GetExpansionCostAmount();
				TerritoryItem.Costs.Add(Cost);
			}
		}
		CachedItems.Add(TerritoryItem);

		if (BuildEntryClass)
		{
			UBuildEntry* Entry = CreateWidget<UBuildEntry>(this, BuildEntryClass);
			if (Entry)
			{
				Entry->Setup(TerritoryItem);
				Entry->OnEntryClicked.AddDynamic(this, &UPanel_Buildings::SelectBuilding);
				if (UWrapBoxSlot* WrapSlot = Cast<UWrapBoxSlot>(WrapBox_BuildEntries->AddChild(Entry)))
				{
					WrapSlot->SetPadding(FMargin(8.f));
				}
			}
		}
	}

	for (const FBuildingDefinitionRow* Row : Rows)
	{
		if (!Row)
		{
			continue;
		}

		UE_LOG(LogTemp, Warning, TEXT("[BuildPanel] BuildingId=%s DisplayName=%s BuildWorkId=%s"), *Row->BuildingId.ToString(), *Row->DisplayName.ToString(), *Row->BuildWorkId.ToString());

		FBuildPanelItem Item;
		if (!BuildItemFromTables(Row->BuildingId, Item))
		{
			continue;
		}

		CachedItems.Add(Item);

		if (!BuildEntryClass)
		{
			UE_LOG(LogTemp, Error, TEXT("[BuildPanel] BuildEntryClass is null"));
			continue;
		}

		UBuildEntry* Entry = CreateWidget<UBuildEntry>(this, BuildEntryClass);
		if (!Entry)
		{
			UE_LOG(LogTemp, Error, TEXT("[BuildPanel] Failed to create build entry widget"));
			continue;
		}

		Entry->Setup(Item);
		Entry->OnEntryClicked.AddDynamic(this, &UPanel_Buildings::SelectBuilding);

		if (UWrapBoxSlot* WrapSlot = Cast<UWrapBoxSlot>(WrapBox_BuildEntries->AddChild(Entry)))
		{
			WrapSlot->SetPadding(FMargin(8.f));
		}
	}

	if (CachedItems.Num() > 0)
	{
		SelectBuilding(CachedItems[0].BuildingId);
	}
}

bool UPanel_Buildings::BuildItemFromTables(FName BuildingId, FBuildPanelItem& OutItem) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	UGameInstance* GI = World->GetGameInstance();
	if (!GI)
	{
		return false;
	}

	UGIS_DataRegistry* Registry = GI->GetSubsystem<UGIS_DataRegistry>();
	if (!Registry)
	{
		return false;
	}

	const FBuildingDefinitionRow* BuildingDef = Registry->GetBuildingDef(BuildingId);
	if (!BuildingDef)
	{
		UE_LOG(LogTemp, Error, TEXT("[BuildPanel] Can't Find Building Def"));
		return false;
	}

	const FWorkDefinitionRow* WorkDef = Registry->GetWorkDef(BuildingDef->BuildWorkId);
	if (!WorkDef)
	{
		return false;
	}

	OutItem.BuildingId = BuildingDef->BuildingId;
	OutItem.DisplayName = BuildingDef->DisplayName;
	OutItem.Description = BuildingDef->Description;
	OutItem.Category = BuildingDef->Category;
	OutItem.ThumbnailIcon = BuildingDef->ThumbnailIcon;
	OutItem.BuildWorkId = BuildingDef->BuildWorkId;
	OutItem.Costs = WorkDef->Costs;
	OutItem.TotalWork = WorkDef->TotalWork;
	OutItem.RequiredResearchIds = BuildingDef->RequiredResearchIds;
	return true;
}

void UPanel_Buildings::SelectBuilding(const FName BuildingId)
{
	SelectedBuildingId = BuildingId;
	UE_LOG(LogTemp, Warning, TEXT("[BuildPanel] Build Selected : %s"), *BuildingId.ToString());
	RebuildDetail();
}

void UPanel_Buildings::RebuildDetail()
{
	const FBuildPanelItem* Item = CachedItems.FindByPredicate([&](const FBuildPanelItem& It)
	{
		return It.BuildingId == SelectedBuildingId;
	});

	if (!Item)
	{
		return;
	}

	if (Text_SelectedName)
	{
		Text_SelectedName->SetText(Item->DisplayName);
	}
	if (Text_SelectedDesc)
	{
		Text_SelectedDesc->SetText(Item->Description);
	}
	if (Text_TotalWorkValue)
	{
		Text_TotalWorkValue->SetText(Item->BuildingId == PanelBuild_TerritoryExpansionBuildId ? FText::FromString(TEXT("Instant")) : FText::AsNumber(Item->TotalWork));
	}
	if (Text_WorkIdValue)
	{
		Text_WorkIdValue->SetText(Item->BuildingId == PanelBuild_TerritoryExpansionBuildId ? FText::FromString(TEXT("Immediate")) : FText::FromName(Item->BuildWorkId));
	}
	if (Text_CategoryValue)
	{
		Text_CategoryValue->SetText(FText::FromString(UEnum::GetValueAsString(Item->Category)));
	}
	if (Image_SelectedIcon)
	{
		if (UTexture2D* Tex = Item->ThumbnailIcon.LoadSynchronous())
		{
			Image_SelectedIcon->SetBrushFromTexture(Tex);
		}
	}

	if (WrapBox_CostList)
	{
		WrapBox_CostList->ClearChildren();
		for (const FWorkCost& Cost : Item->Costs)
		{
			UTextBlock* Txt = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			Txt->SetText(FText::FromString(FString::Printf(TEXT("%s x%d"), *Cost.ResourceId.ToString(), Cost.Amount)));
			WrapBox_CostList->AddChildToWrapBox(Txt);
		}

		if (Item->BuildingId == PanelBuild_TerritoryExpansionBuildId && WrapBox_CostList->GetChildrenCount() == 0)
		{
			UTextBlock* Txt = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			Txt->SetText(FText::FromString(TEXT("No resource cost configured")));
			WrapBox_CostList->AddChildToWrapBox(Txt);
		}
	}

	if (VerticalBox_RequirementList)
	{
		VerticalBox_RequirementList->ClearChildren();
		for (const FName& Req : Item->RequiredResearchIds)
		{
			UTextBlock* Txt = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			Txt->SetText(FText::FromName(Req));
			VerticalBox_RequirementList->AddChildToVerticalBox(Txt);
		}
	}
}

void UPanel_Buildings::HandleStartBuildClicked()
{
	if (!SelectedBuildingId.IsNone())
	{
		OnBuildStartRequested.Broadcast(SelectedBuildingId);
	}
}

