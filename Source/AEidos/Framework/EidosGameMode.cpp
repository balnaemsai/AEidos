// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/EidosGameMode.h"

#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

void AEidosGameMode::TriggerGameOver()
{
	if (bGameOver || bGameVictory)
	{
		return;
	}

	bGameOver = true;
	UE_LOG(LogTemp, Error, TEXT("[GameMode] Game over: settlement core destroyed."));
	OnGameOver.Broadcast();

	// A core loss must be visible and must stop new player commands even before the
	// dedicated game-over modal is authored in UMG.
	if (UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			if (APlayerController* PlayerController = It->Get())
			{
				PlayerController->SetIgnoreMoveInput(true);
				PlayerController->SetIgnoreLookInput(true);
				PlayerController->bShowMouseCursor = true;
			}
		}
		UGameplayStatics::SetGamePaused(World, true);
	}
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			INDEX_NONE,
			10.f,
			FColor(230, 96, 96),
			TEXT("SETTLEMENT DEFEATED - THE CORE HAS BEEN DESTROYED"));
	}
}

void AEidosGameMode::TriggerGameVictory(const FText& ScenarioName, const FText& ScenarioDescription)
{
	if (bGameOver || bGameVictory)
	{
		return;
	}

	bGameVictory = true;
	VictoryScenarioName = ScenarioName;
	VictoryScenarioDescription = ScenarioDescription;
	UE_LOG(LogTemp, Log, TEXT("[GameMode] Scenario completed: %s"), *ScenarioName.ToString());
	OnGameVictory.Broadcast(ScenarioName, ScenarioDescription);

	if (UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			if (APlayerController* PlayerController = It->Get())
			{
				PlayerController->SetIgnoreMoveInput(true);
				PlayerController->SetIgnoreLookInput(true);
				PlayerController->bShowMouseCursor = true;
			}
		}
		UGameplayStatics::SetGamePaused(World, true);
	}
}

