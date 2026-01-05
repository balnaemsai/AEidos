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

	UFUNCTION(BlueprintCallable)
	void ShowMainMenu();

	void RequestNewGame();
	void RequestLoadGame(const FString& SlotName, int32 UserIndex);

protected:
	
	virtual void BeginPlay() override;

private:

	void OpenGameMap();

	static constexpr const TCHAR* GameMapName = TEXT("GameMap");

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> MainMenuWidget;
	
};
