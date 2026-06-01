// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/HUD/Panels/PageEntry.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UPageEntry::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Root)
	{
		Button_Root->OnClicked.AddDynamic(this, &UPageEntry::HandleClicked);
	}
}

void UPageEntry::Setup(const FPageSummaryView& InView)
{
	ViewData = InView;

	if (Text_Name)
	{
		Text_Name->SetText(ViewData.DisplayName);
	}

	if (Image_DungeonState)
	{
		Image_DungeonState->SetVisibility(ViewData.bIsInDungeon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (Image_PrisonerState)
	{
		// Prisoner/captive state is not implemented yet, so keep the slot hidden for now.
		Image_PrisonerState->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (Border_Selection)
	{
		Border_Selection->SetBrushColor(ViewData.bIsSelected ? FLinearColor(0.95f, 0.8f, 0.25f, 1.f) : FLinearColor(0.12f, 0.12f, 0.12f, 0.85f));
	}
}

void UPageEntry::HandleClicked()
{
	OnEntryClicked.Broadcast(ViewData.PageId);
}
