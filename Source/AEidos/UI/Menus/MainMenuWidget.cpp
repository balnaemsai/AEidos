// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Menus/MainMenuWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
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

	SetStatus(TEXT("Ready"));
}

void UMainMenuWidget::OnClickNewGame()
{
	SetStatus(TEXT("Preparing New Game..."));

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UGIS_SaveLoad* SaveLoad = GI->GetSubsystem<UGIS_SaveLoad>())
		{
			// 최소 구현: “새 게임 스냅샷”만 준비
			/*
			const bool bOk = SaveLoad->PrepareNewGameSnapshot(); // 너가 만들 함수
			if (!bOk)
			{
				SetStatus(TEXT("New Game failed."));
				return;
			}
			*/

			SetStatus(TEXT("Opening GameMap..."));
			UGameplayStatics::OpenLevel(this, FName("GameMap"));
			return;
		}
	}

	SetStatus(TEXT("SaveLoad subsystem missing."));
}

void UMainMenuWidget::OnClickLoadGame()
{
	SetStatus(TEXT("Preparing Load..."));

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UGIS_SaveLoad* SaveLoad = GI->GetSubsystem<UGIS_SaveLoad>())
		{
			/*
			const bool bOk = SaveLoad->PrepareLoadSnapshot_Last(); // 너가 만들 함수
			if (!bOk)
			{
				SetStatus(TEXT("Load failed."));
				return;
			}
			*/

			SetStatus(TEXT("Opening GameMap..."));
			UGameplayStatics::OpenLevel(this, FName("GameMap"));
			return;
		}
	}

	SetStatus(TEXT("SaveLoad subsystem missing."));
}

void UMainMenuWidget::OnClickOptions()
{
	SetStatus(TEXT("Options (TODO)"));
}

void UMainMenuWidget::OnClickQuit()
{
	UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, true);
}

void UMainMenuWidget::SetStatus(const FString& Msg)
{
	if (TxtStatus)
	{
		TxtStatus->SetText(FText::FromString(Msg));
	}
}

