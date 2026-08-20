// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MenuPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class AEIDOS_API AMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UUserWidget> MainMenuWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<UUserWidget> ScenarioSelectionWidgetClass;

	UFUNCTION(BlueprintCallable)
	void ShowMainMenu();

	UFUNCTION(BlueprintCallable)
	void ShowScenarioSelection();

	/** Restores the original menu after the scenario picker is closed. */
	void FocusMainMenu();

	void RequestNewGame();
	/** Returns false without leaving the menu when the requested save cannot be loaded. */
	bool RequestLoadGame(const FString& SlotName, int32 UserIndex);

protected:
	
	virtual void BeginPlay() override;

private:

	void OpenGameMap();

	static constexpr const TCHAR* GameMapName = TEXT("GameMap");

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> MainMenuWidget;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ScenarioSelectionWidget;
	
};
