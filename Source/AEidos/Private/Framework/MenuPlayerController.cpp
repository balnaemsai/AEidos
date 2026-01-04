// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/MenuPlayerController.h"
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

