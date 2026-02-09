// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GIS_UIRouter.generated.h"

UENUM(BlueprintType)
enum class EUIState : uint8
{
	None        UMETA(DisplayName="None"),

	MainMenu   UMETA(DisplayName="Main Menu"),
	Loading    UMETA(DisplayName="Loading"),
	InGame     UMETA(DisplayName="In Game"),
	Paused     UMETA(DisplayName="Paused"),

	Modal      UMETA(DisplayName="Modal"),
};

/**
 * 
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUIStateChanged, EUIState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEnterInGame);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnExitInGame);

UCLASS()
class AEIDOS_API UGIS_UIRouter : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="UI")
	void RequestUIState(EUIState NewState);

	UFUNCTION(BlueprintCallable, Category="UI")
	EUIState GetCurrentUIState() const { return CurrentState; }

	UPROPERTY(BlueprintAssignable)
	FOnUIStateChanged OnUIStateChanged;

	UPROPERTY(BlueprintAssignable)
	FOnEnterInGame OnEnterInGame;

	UPROPERTY(BlueprintAssignable)
	FOnExitInGame OnExitInGame;

private:
	UPROPERTY()
	EUIState CurrentState = EUIState::None;
	
};
