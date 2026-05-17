// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/EidosHUD.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

#include "UI/GIS_UIRouter.h"

void AEidosHUD::BeginPlay()
{
	Super::BeginPlay();
	
	if (UGameInstance* GI = GetGameInstance())
	{
		UIRouter = GI->GetSubsystem<UGIS_UIRouter>();
	}

	if (UIRouter)
	{
		UIRouter->OnUIStateChanged.AddDynamic(this, &AEidosHUD::HandleUIStateChanged);
		HandleUIStateChanged(UIRouter->GetCurrentUIState());
	}
}

void AEidosHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UIRouter)
	{
		UIRouter->OnUIStateChanged.RemoveDynamic(this, &AEidosHUD::HandleUIStateChanged);
		UIRouter = nullptr;
	}

	DestroyHUDRoot();
	Super::EndPlay(EndPlayReason);
}

void AEidosHUD::HandleUIStateChanged(EUIState NewState)
{
	if (NewState == EUIState::InGame)
	{
		EnsureHUDRoot();
	}
	else
	{
		DestroyHUDRoot();
	}
}

void AEidosHUD::EnsureHUDRoot()
{
	if (HUDRootWidgetInstance || !HUDRootWidgetClass)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (APlayerController* OwningPC = GetOwningPlayerController())
	{
		HUDRootWidgetInstance = CreateWidget<UUserWidget>(OwningPC, HUDRootWidgetClass);
	}
	else
	{
		HUDRootWidgetInstance = CreateWidget<UUserWidget>(World, HUDRootWidgetClass);
	}
	if (HUDRootWidgetInstance)
	{
		HUDRootWidgetInstance->AddToViewport(0);
	}
}

void AEidosHUD::DestroyHUDRoot()
{
	if (!HUDRootWidgetInstance)
		return;

	HUDRootWidgetInstance->RemoveFromParent();
	HUDRootWidgetInstance = nullptr;
}
