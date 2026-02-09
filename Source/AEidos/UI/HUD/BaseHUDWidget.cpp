// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/BaseHUDWidget.h"
#include "UI/HUD/BaseHUDWidget.h"
#include "Engine/World.h"
#include "UI/HUD/ResourcePanelWidget.h"
#include "World/Settlement/WS_Economy.h"

void UBaseHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindEconomy();
	RefreshResourcePanel(); 
}

void UBaseHUDWidget::NativeDestruct()
{
	UnbindEconomy();
	Super::NativeDestruct();
}

void UBaseHUDWidget::BindEconomy()
{
	if (!GetWorld()) return;

	Economy = GetWorld()->GetSubsystem<UWS_Economy>();
	if (!Economy) return;

	// ✅ Dynamic multicast delegate 가정
	Economy->OnEconomyChanged.AddDynamic(this, &UBaseHUDWidget::HandleEconomyChanged);
}

void UBaseHUDWidget::UnbindEconomy()
{
	if (!Economy) return;

	Economy->OnEconomyChanged.RemoveDynamic(this, &UBaseHUDWidget::HandleEconomyChanged);
	Economy = nullptr;
}

void UBaseHUDWidget::HandleEconomyChanged()
{
	RefreshResourcePanel();
}

void UBaseHUDWidget::RefreshResourcePanel()
{
	if (!ResourcePanel || !GetWorld()) return;

	UWS_Economy* Eco = Economy ? Economy : GetWorld()->GetSubsystem<UWS_Economy>();
	if (!Eco) return;
	
	const int32 Food  = Eco->GetAmount(FName("Food"));
	const int32 Wood  = Eco->GetAmount(FName("Wood"));
	const int32 Stone = Eco->GetAmount(FName("Stone"));
	const int32 Metal = Eco->GetAmount(FName("Metal"));

	ResourcePanel->SetResources(Food, Wood, Stone, Metal);
}