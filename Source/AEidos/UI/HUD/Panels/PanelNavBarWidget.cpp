// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/HUD/Panels/PanelNavBarWidget.h"
#include "Components/Button.h"
#include "Styling/SlateTypes.h"
#include "Math/Color.h"

void UPanelNavBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	InitStylesFrom(BtnBuildings ? BtnBuildings : BtnPages);

	if (BtnBuildings) BtnBuildings->OnClicked.AddDynamic(this, &UPanelNavBarWidget::HandleBuildings);
	if (BtnPages) BtnPages->OnClicked.AddDynamic(this, &UPanelNavBarWidget::HandlePages);
	if (BtnDungeons) BtnDungeons->OnClicked.AddDynamic(this, &UPanelNavBarWidget::HandleDungeons);
	if (BtnItems) BtnItems->OnClicked.AddDynamic(this, &UPanelNavBarWidget::HandleItems);
	if (BtnResearch) BtnResearch->OnClicked.AddDynamic(this, &UPanelNavBarWidget::HandleResearch);

	Refresh(ActivePanel);
}

void UPanelNavBarWidget::InitStylesFrom(UButton* AnyButton)
{
	if (bStylesInitialized || !AnyButton)
	{
		return;
	}

	InactiveStyle = AnyButton->GetStyle();
	ActiveStyle = InactiveStyle;
	ActiveStyle.SetNormal(BrightenBrush(InactiveStyle.Normal, ActiveBrightnessMul, ActiveSaturationMul));
	ActiveStyle.SetHovered(BrightenBrush(InactiveStyle.Hovered, ActiveBrightnessMul, ActiveSaturationMul));
	ActiveStyle.SetPressed(BrightenBrush(InactiveStyle.Pressed, ActiveBrightnessMul, ActiveSaturationMul));
	ActiveStyle.SetDisabled(InactiveStyle.Disabled);
	bStylesInitialized = true;
}

FSlateBrush UPanelNavBarWidget::BrightenBrush(const FSlateBrush& InBrush, float BrightMul, float SatMul)
{
	FSlateBrush Out = InBrush;
	const FLinearColor Base = InBrush.TintColor.GetSpecifiedColor();
	FLinearColor HSV = Base.LinearRGBToHSV();
	HSV.G = FMath::Clamp(HSV.G * SatMul, 0.0f, 1.0f);
	HSV.B = FMath::Clamp(HSV.B * BrightMul, 0.0f, 1.0f);
	Out.TintColor = FSlateColor(HSV.HSVToLinearRGB());
	return Out;
}

void UPanelNavBarWidget::Apply(UButton* Btn, bool bActive)
{
	if (!Btn)
	{
		return;
	}
	InitStylesFrom(Btn);
	Btn->SetStyle(bActive ? ActiveStyle : InactiveStyle);
}

void UPanelNavBarWidget::Refresh(EInGamePanel Panel)
{
	ActivePanel = Panel;
	Apply(BtnBuildings, Panel == EInGamePanel::Buildings);
	Apply(BtnPages, Panel == EInGamePanel::Pages);
	Apply(BtnDungeons, Panel == EInGamePanel::Dungeons);
	Apply(BtnItems, Panel == EInGamePanel::Items);
	Apply(BtnResearch, Panel == EInGamePanel::Research);
}

void UPanelNavBarWidget::SetActivePanel(EInGamePanel Panel)
{
	Refresh(Panel);
}

void UPanelNavBarWidget::HandleBuildings() { OnPanelSelected.Broadcast(EInGamePanel::Buildings); }
void UPanelNavBarWidget::HandlePages() { OnPanelSelected.Broadcast(EInGamePanel::Pages); }
void UPanelNavBarWidget::HandleDungeons() { OnPanelSelected.Broadcast(EInGamePanel::Dungeons); }
void UPanelNavBarWidget::HandleItems() { OnPanelSelected.Broadcast(EInGamePanel::Items); }
void UPanelNavBarWidget::HandleResearch() { OnPanelSelected.Broadcast(EInGamePanel::Research); }
