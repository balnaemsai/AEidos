// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/ResourcePanelWidget.h"
#include "Components/TextBlock.h"

void UResourcePanelWidget::SetResources(int32 Food, int32 Wood, int32 Stone, int32 Metal)
{
	if (TxtFood)  TxtFood->SetText(FText::AsNumber(Food));
	if (TxtWood)  TxtWood->SetText(FText::AsNumber(Wood));
	if (TxtStone) TxtStone->SetText(FText::AsNumber(Stone));
	if (TxtMetal) TxtMetal->SetText(FText::AsNumber(Metal));
}

