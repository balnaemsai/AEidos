// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/MenuPlayerController.h"
#include "UI/Menus/ScenarioSelectionWidget.h"
#include "UI/Menus/MainMenuWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Save/GIS_SaveLoad.h"
#include "UI/GIS_UIRouter.h"
#include "Blueprint/UserWidget.h"

void AMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UGIS_UIRouter* UIRouter = GameInstance->GetSubsystem<UGIS_UIRouter>())
		{
			UIRouter->RequestUIState(EUIState::MainMenu);
		}
	}

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

void AMenuPlayerController::ShowScenarioSelection()
{
	if (ScenarioSelectionWidget)
	{
		return;
	}

	if (!ScenarioSelectionWidgetClass)
	{
		ScenarioSelectionWidgetClass = LoadClass<UScenarioSelectionWidget>(nullptr,
			TEXT("/Game/Blueprints/WBP/WBP_ScenarioSelection.WBP_ScenarioSelection_C"));
	}
	if (!ScenarioSelectionWidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[Scenario] ScenarioSelectionWidgetClass is not assigned."));
		return;
	}

	ScenarioSelectionWidget = CreateWidget<UUserWidget>(this, ScenarioSelectionWidgetClass);
	if (!ScenarioSelectionWidget)
	{
		return;
	}

	if (MainMenuWidget)
	{
		MainMenuWidget->SetVisibility(ESlateVisibility::Hidden);
	}
	ScenarioSelectionWidget->AddToViewport(10);

	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetWidgetToFocus(ScenarioSelectionWidget->TakeWidget());
	SetInputMode(InputMode);
	bShowMouseCursor = true;
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

	SL->StartNewGame(GameMapName);

	OpenGameMap();
}

bool AMenuPlayerController::RequestLoadGame(const FString& SlotName, int32 UserIndex)
{
	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return false;
	}

	UGIS_SaveLoad* SL = GI->GetSubsystem<UGIS_SaveLoad>();
	if (!SL)
	{
		return false;
	}
	
	const bool bOk = SL->LoadFromSlotToPending(SlotName, UserIndex);
	if (!bOk)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Save] Load request rejected: slot '%s' does not contain a valid save."), *SlotName);
		return false;
	}

	OpenGameMap();
	return true;
}

void AMenuPlayerController::OpenGameMap()
{
	UGameplayStatics::OpenLevel(this, FName(GameMapName));
}

void AMenuPlayerController::FocusMainMenu()
{
	if (ScenarioSelectionWidget)
	{
		ScenarioSelectionWidget->RemoveFromParent();
		ScenarioSelectionWidget = nullptr;
	}
	if (MainMenuWidget)
	{
		MainMenuWidget->SetVisibility(ESlateVisibility::Visible);
	}

	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	if (MainMenuWidget)
	{
		InputMode.SetWidgetToFocus(MainMenuWidget->TakeWidget());
	}
	SetInputMode(InputMode);
	bShowMouseCursor = true;
}



