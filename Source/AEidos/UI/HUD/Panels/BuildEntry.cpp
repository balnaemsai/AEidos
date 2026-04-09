// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/Panels/BuildEntry.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Core/Types/WorkTypes.h" //FWorkCost
#include "Engine/Texture2D.h"

void UBuildEntry::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Root)
	{
		Button_Root->OnClicked.AddDynamic(this, &UBuildEntry::HandleClicked);
	}
}

void UBuildEntry::Setup(const FBuildPanelItem& InItem)
{
	ItemData = InItem;

	if (Text_Name)
	{
		Text_Name->SetText(ItemData.DisplayName);
	}
	if (Text_Category)
	{
		Text_Category->SetText(FText::FromString(UEnum::GetValueAsString(ItemData.Category)));
	}
	if (Image_Icon)
	{
		if (UTexture2D* Tex = ItemData.ThumbnailIcon.LoadSynchronous())
		{
			Image_Icon->SetBrushFromTexture(Tex);
		}
	}
}

void UBuildEntry::HandleClicked()
{
	OnEntryClicked.Broadcast(ItemData.BuildingId);
}