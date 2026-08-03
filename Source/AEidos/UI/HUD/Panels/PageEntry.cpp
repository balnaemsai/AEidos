// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/HUD/Panels/PageEntry.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

namespace
{
	FLinearColor UiColor(const TCHAR* Hex, float Alpha = 1.f)
	{
		FLinearColor Color = FLinearColor::FromSRGBColor(FColor::FromHex(Hex));
		Color.A = Alpha;
		return Color;
	}
}

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
		Border_Selection->SetBrushColor(ViewData.bIsSelected
			? UiColor(TEXT("D7C9AE"), 0.65f)
			: UiColor(TEXT("DCE0E2"), 0.14f));
	}
}

void UPageEntry::HandleClicked()
{
	OnEntryClicked.Broadcast(ViewData.PageId);
}
