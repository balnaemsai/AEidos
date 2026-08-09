// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/Panels/Panel_Research.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "TimerManager.h"
#include "UI/HUD/PanelUIFunctionLibrary.h"
#include "UI/HUD/Panels/ResearchEntry.h"
#include "World/Settlement/WS_Research.h"

void UPanel_Research::NativeConstruct()
{
	Super::NativeConstruct();
	if (!ResearchEntryClass) ResearchEntryClass = LoadClass<UResearchEntry>(nullptr, TEXT("/Game/Blueprints/WBP/WBP_ResearchEntry.WBP_ResearchEntry_C"));
	if (Button_Close) Button_Close->OnClicked.AddDynamic(this, &UPanel_Research::HandleCloseClicked);
	ObservedResearch = GetWorld() ? GetWorld()->GetSubsystem<UWS_Research>() : nullptr;
	if (UWS_Research* Research = ObservedResearch.Get()) ResearchChangedHandle = Research->OnResearchChanged.AddUObject(this, &UPanel_Research::RefreshResearchList);
	RefreshResearchList();
}

void UPanel_Research::NativeDestruct()
{
	StopProgressRefresh();
	if (UWS_Research* Research = ObservedResearch.Get()) Research->OnResearchChanged.Remove(ResearchChangedHandle);
	ResearchChangedHandle.Reset();
	ObservedResearch.Reset();
	Super::NativeDestruct();
}

void UPanel_Research::OnPanelShown_Implementation()
{
	RefreshResearchList();
	StartProgressRefresh();
}

void UPanel_Research::OnPanelHidden_Implementation()
{
	StopProgressRefresh();
}

void UPanel_Research::RefreshResearchList()
{
	if (!VerticalBox_ResearchEntries) return;
	VerticalBox_ResearchEntries->ClearChildren();
	ResearchEntriesById.Reset();
	UWS_Research* Research = GetWorld() ? GetWorld()->GetSubsystem<UWS_Research>() : nullptr;
	const TArray<FResearchView> Views = Research ? Research->GetResearchViews() : TArray<FResearchView>();
	for (const FResearchView& View : Views)
	{
		if (UResearchEntry* Entry = CreateWidget<UResearchEntry>(this, ResearchEntryClass))
		{
			Entry->Setup(View);
			ResearchEntriesById.Add(View.ResearchId, Entry);
			Entry->OnResearchStartRequested.AddDynamic(this, &UPanel_Research::HandleStartResearch);
			Entry->OnResearchCancelRequested.AddDynamic(this, &UPanel_Research::HandleCancelResearch);
			if (UVerticalBoxSlot* EntrySlot = VerticalBox_ResearchEntries->AddChildToVerticalBox(Entry)) EntrySlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
		}
	}
	if (Text_Empty) Text_Empty->SetVisibility(Views.IsEmpty() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UPanel_Research::RefreshResearchProgress()
{
	UWS_Research* Research = GetWorld() ? GetWorld()->GetSubsystem<UWS_Research>() : nullptr;
	if (!Research || ResearchEntriesById.IsEmpty())
	{
		return;
	}

	for (const FResearchView& View : Research->GetResearchViews())
	{
		if (TObjectPtr<UResearchEntry>* Entry = ResearchEntriesById.Find(View.ResearchId))
		{
			if (UResearchEntry* EntryWidget = Entry->Get())
			{
				EntryWidget->Setup(View);
			}
		}
	}
}

void UPanel_Research::StartProgressRefresh()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(ProgressRefreshTimer, this, &UPanel_Research::RefreshResearchProgress, 0.25f, true);
	}
}

void UPanel_Research::StopProgressRefresh()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ProgressRefreshTimer);
	}
}

void UPanel_Research::HandleStartResearch(FName ResearchId)
{
	if (UWS_Research* Research = GetWorld() ? GetWorld()->GetSubsystem<UWS_Research>() : nullptr) Research->StartResearch(ResearchId);
}

void UPanel_Research::HandleCancelResearch(FName ResearchId)
{
	if (UWS_Research* Research = GetWorld() ? GetWorld()->GetSubsystem<UWS_Research>() : nullptr) Research->CancelResearch(ResearchId);
}

void UPanel_Research::HandleCloseClicked()
{
	UPanelUIFunctionLibrary::ClosePanel(this);
}

