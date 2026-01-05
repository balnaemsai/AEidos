// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/MenuPlayerController.h"
#include "UI/Menus/MainMenuWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Save/GIS_SaveLoad.h"
#include "Blueprint/UserWidget.h"

void AMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();
	ShowMainMenu();
}

void AMenuPlayerController::ShowMainMenu()
{
	if (MainMenuWidget || !MainMenuWidgetClass)
		return;

	MainMenuWidget = CreateWidget<UUserWidget>(this, MainMenuWidgetClass);
	if (!MainMenuWidget)
		return;

	MainMenuWidget->AddToViewport(0);
	
	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetWidgetToFocus(MainMenuWidget->TakeWidget());
	SetInputMode(InputMode);

	bShowMouseCursor = true;
	SetIgnoreLookInput(true);
	SetIgnoreMoveInput(true);
}

void AMenuPlayerController::RequestNewGame()
{
	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return;
	}

	UGIS_SaveLoad* SL = GI->GetSubsystem<UGIS_SaveLoad>();
	if (!SL)
	{
		return;
	}

	SL->ClearPendingSnapshot();
	SL->BuildNewGameSnapshotIfNeeded(GameMapName);

	OpenGameMap();
}

void AMenuPlayerController::RequestLoadGame(const FString& SlotName, int32 UserIndex)
{
	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return;
	}

	UGIS_SaveLoad* SL = GI->GetSubsystem<UGIS_SaveLoad>();
	if (!SL)
	{
		return;
	}
	
	const bool bOk = SL->LoadFromSlotToPending(SlotName, UserIndex);

	OpenGameMap();
}

void AMenuPlayerController::OpenGameMap()
{
	UGameplayStatics::OpenLevel(this, FName(GameMapName));
}



