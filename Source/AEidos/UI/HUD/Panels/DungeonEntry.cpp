#include "UI/HUD/Panels/DungeonEntry.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

namespace
{
	FLinearColor DungeonEntryUiColor(const TCHAR* Hex, float Alpha = 1.f)
	{
		FLinearColor Color = FLinearColor::FromSRGBColor(FColor::FromHex(Hex));
		Color.A = Alpha;
		return Color;
	}
}

void UDungeonEntry::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Root)
	{
		Button_Root->OnClicked.AddDynamic(this, &UDungeonEntry::HandleClicked);
	}
}

void UDungeonEntry::Setup(const FDungeonPortalView& InView)
{
	ViewData = InView;

	if (Text_Title)
	{
		Text_Title->SetText(ViewData.DisplayName);
	}

	if (Text_Subtitle)
	{
		Text_Subtitle->SetText(FText::Format(FText::FromString(TEXT("{0} · {1}")), ViewData.DifficultyText, ViewData.StatusText));
	}
	if (Text_Timer)
	{
		Text_Timer->SetText(ViewData.RaidTimerText);
	}

	if (Image_Status)
	{
		Image_Status->SetColorAndOpacity(ViewData.StatusColor);
	}

	if (Border_Selection)
	{
		Border_Selection->SetBrushColor(ViewData.bIsSelected
			? DungeonEntryUiColor(TEXT("D7C9AE"), 0.65f)
			: DungeonEntryUiColor(TEXT("DCE0E2"), 0.14f));
	}
}

void UDungeonEntry::HandleClicked()
{
	OnEntryClicked.Broadcast(ViewData.PortalId);
}
