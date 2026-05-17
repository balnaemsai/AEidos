// Fill out your copyright notice in the Description page of Project Settings.



#include "UI/HUD/Panels/PanelNavBarWidget.h"
#include "Components/Button.h"
#include "Styling/SlateTypes.h"
#include "Math/Color.h"

void UPanelNavBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	InitStylesFrom(BtnRecruit);

	if (BtnRecruit)   BtnRecruit->OnClicked.AddDynamic(this, &UPanelNavBarWidget::HandleRecruit);
	if (BtnCraft)     BtnCraft->OnClicked.AddDynamic(this, &UPanelNavBarWidget::HandleCraft);
	if (BtnResearch)  BtnResearch->OnClicked.AddDynamic(this, &UPanelNavBarWidget::HandleResearch);
	if (BtnBuild)     BtnBuild->OnClicked.AddDynamic(this, &UPanelNavBarWidget::HandleBuild);
	if (BtnBuildings)     BtnBuildings->OnClicked.AddDynamic(this, &UPanelNavBarWidget::HandleBuildings);
	if (BtnDungeons)     BtnDungeons->OnClicked.AddDynamic(this, &UPanelNavBarWidget::HandleDungeons);
	if (BtnPages)     BtnPages->OnClicked.AddDynamic(this, &UPanelNavBarWidget::HandlePages);
	if (BtnItems)     BtnItems->OnClicked.AddDynamic(this, &UPanelNavBarWidget::HandleItems);
	if (BtnRelations) BtnRelations->OnClicked.AddDynamic(this, &UPanelNavBarWidget::HandleRelations);
	if (BtnSkill)     BtnSkill->OnClicked.AddDynamic(this, &UPanelNavBarWidget::HandleSkill);
	
}

void UPanelNavBarWidget::InitStylesFrom(UButton* AnyButton)
{
	if (bStylesInitialized)
		return;

	if (!AnyButton)
		return;
	
	InactiveStyle = AnyButton->GetStyle();
	ActiveStyle = InactiveStyle;

	ActiveStyle.SetNormal   (BrightenBrush(InactiveStyle.Normal,   ActiveBrightnessMul, ActiveSaturationMul));
	ActiveStyle.SetHovered  (BrightenBrush(InactiveStyle.Hovered,  ActiveBrightnessMul, ActiveSaturationMul));
	ActiveStyle.SetPressed  (BrightenBrush(InactiveStyle.Pressed,  ActiveBrightnessMul, ActiveSaturationMul));
	ActiveStyle.SetDisabled (InactiveStyle.Disabled); 

	bStylesInitialized = true;
}

FSlateBrush UPanelNavBarWidget::BrightenBrush(const FSlateBrush& InBrush, float BrightMul, float SatMul)
{
	FSlateBrush Out = InBrush;
	const FLinearColor Base = InBrush.TintColor.GetSpecifiedColor();
	FLinearColor HSV = Base.LinearRGBToHSV();

	HSV.G = FMath::Clamp(HSV.G * SatMul, 0.0f, 1.0f); // S
	HSV.B = FMath::Clamp(HSV.B * BrightMul, 0.0f, 1.0f); // V

	const FLinearColor NewRGB = HSV.HSVToLinearRGB();

	Out.TintColor = FSlateColor(NewRGB);
	return Out;
}

void UPanelNavBarWidget::Apply(UButton* Btn, bool bActive)
{
	if (!Btn) return;
	InitStylesFrom(Btn);
	Btn->SetStyle(bActive ? ActiveStyle : InactiveStyle);
}

void UPanelNavBarWidget::Refresh(EInGamePanel Panel)
{
	ActivePanel = Panel;
	Apply(BtnRecruit,   Panel == EInGamePanel::Recruit);
	Apply(BtnCraft,     Panel == EInGamePanel::Craft);
	Apply(BtnResearch,  Panel == EInGamePanel::Research);
	Apply(BtnBuild,     Panel == EInGamePanel::Build);
	Apply(BtnBuildings, Panel == EInGamePanel::Buildings);
	Apply(BtnDungeons,  Panel == EInGamePanel::Dungeons);
	Apply(BtnPages,     Panel == EInGamePanel::Pages);
	Apply(BtnItems,     Panel == EInGamePanel::Items);
	Apply(BtnRelations, Panel == EInGamePanel::Relations);
	Apply(BtnSkill,     Panel == EInGamePanel::Skill);
}

void UPanelNavBarWidget::SetActivePanel(EInGamePanel Panel)
{
	Refresh(Panel);
}

void UPanelNavBarWidget::HandleRecruit()   { OnPanelSelected.Broadcast(EInGamePanel::Recruit); }
void UPanelNavBarWidget::HandleCraft()     { OnPanelSelected.Broadcast(EInGamePanel::Craft); }
void UPanelNavBarWidget::HandleResearch()  { OnPanelSelected.Broadcast(EInGamePanel::Research); }
void UPanelNavBarWidget::HandleBuild()     { OnPanelSelected.Broadcast(EInGamePanel::Build); }
void UPanelNavBarWidget::HandleBuildings()     { OnPanelSelected.Broadcast(EInGamePanel::Buildings); }
void UPanelNavBarWidget::HandleDungeons()     { OnPanelSelected.Broadcast(EInGamePanel::Dungeons); }
void UPanelNavBarWidget::HandlePages()     { OnPanelSelected.Broadcast(EInGamePanel::Pages); }
void UPanelNavBarWidget::HandleItems()     { OnPanelSelected.Broadcast(EInGamePanel::Items); }
void UPanelNavBarWidget::HandleRelations() { OnPanelSelected.Broadcast(EInGamePanel::Relations); }
void UPanelNavBarWidget::HandleSkill()     { OnPanelSelected.Broadcast(EInGamePanel::Skill); }
