// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Menus/MainMenuWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Framework/MenuPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Save/GIS_SaveLoad.h"
#include "UI/GIS_UIRouter.h"

void UMainMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (BtnNewGame)  BtnNewGame->OnClicked.AddDynamic(this, &UMainMenuWidget::OnClickNewGame);
	if (BtnLoadGame) BtnLoadGame->OnClicked.AddDynamic(this, &UMainMenuWidget::OnClickLoadGame);
	if (BtnOptions)  BtnOptions->OnClicked.AddDynamic(this, &UMainMenuWidget::OnClickOptions);
	if (BtnQuit)     BtnQuit->OnClicked.AddDynamic(this, &UMainMenuWidget::OnClickQuit);

	SetBusy(false, TEXT("Ready"));
}

void UMainMenuWidget::OnClickNewGame()
{
	if (AMenuPlayerController* MPC = Cast<AMenuPlayerController>(GetOwningPlayer()))
	{
		MPC->ShowScenarioSelection();
		return;
	}

	SetBusy(false, TEXT("Error: Invalid Controller"));
}

void UMainMenuWidget::OnClickLoadGame()
{
	SetBusy(true, TEXT("Preparing Load..."));

	if (AMenuPlayerController* MPC = Cast<AMenuPlayerController>(GetOwningPlayer()))
	{
		if (!MPC->RequestLoadGame(TEXT("Slot0"), 0))
		{
			SetBusy(false, TEXT("No valid save was found in Slot 0."));
		}
		return;
	}

	SetBusy(false, TEXT("Error: invalid controller"));
}

void UMainMenuWidget::OnClickOptions()
{
	SetBusy(false, TEXT("Options (TODO)"));
}

void UMainMenuWidget::OnClickQuit()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, true);
}

void UMainMenuWidget::SetBusy(bool bBusy, const FString& StatusText)
{
	if (LoadingDim)
	{
		LoadingDim->SetVisibility(bBusy ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
	if (TxtStatus)
	{
		TxtStatus->SetText(FText::FromString(StatusText));
	}
	if (BtnNewGame) BtnNewGame->SetIsEnabled(!bBusy);
	if (BtnLoadGame) BtnLoadGame->SetIsEnabled(!bBusy);
	if (BtnQuit) BtnQuit->SetIsEnabled(!bBusy);
}

