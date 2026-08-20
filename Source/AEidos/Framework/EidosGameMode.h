// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "EidosGameMode.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameOver);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGameVictory, FText, ScenarioName, FText, ScenarioDescription);

/**
 * 
 */
UCLASS()
class AEIDOS_API AEidosGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Game")
	void TriggerGameOver();

	UFUNCTION(BlueprintCallable, Category="Game")
	void TriggerGameVictory(const FText& ScenarioName, const FText& ScenarioDescription);

	UFUNCTION(BlueprintPure, Category="Game")
	bool IsGameOver() const { return bGameOver; }

	UFUNCTION(BlueprintPure, Category="Game")
	bool IsGameVictory() const { return bGameVictory; }

	UFUNCTION(BlueprintPure, Category="Game")
	FText GetVictoryScenarioName() const { return VictoryScenarioName; }

	UFUNCTION(BlueprintPure, Category="Game")
	FText GetVictoryScenarioDescription() const { return VictoryScenarioDescription; }

	UPROPERTY(BlueprintAssignable, Category="Game")
	FOnGameOver OnGameOver;

	UPROPERTY(BlueprintAssignable, Category="Game")
	FOnGameVictory OnGameVictory;

private:
	bool bGameOver = false;
	bool bGameVictory = false;
	FText VictoryScenarioName;
	FText VictoryScenarioDescription;
};
