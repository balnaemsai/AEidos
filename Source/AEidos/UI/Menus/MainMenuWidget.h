// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuWidget.generated.h"

class UButton;
class UTextBlock;
class UImage;

/**
 * 
 */
UCLASS()
class AEIDOS_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

	protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidget))
	UButton* BtnNewGame;
	UPROPERTY(meta = (BindWidget))
	UButton* BtnLoadGame;
	UPROPERTY(meta = (BindWidget))
	UButton* BtnOptions;
	UPROPERTY(meta = (BindWidget))
	UButton* BtnQuit;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TxtStatus;
	UPROPERTY(meta = (BindWidgetOptional))
	UImage* LoadingDim;

	private:
	UFUNCTION()
	void OnClickNewGame();
	UFUNCTION()
	void OnClickLoadGame();
	UFUNCTION()
	void OnClickOptions();
	UFUNCTION()
	void OnClickQuit();

	void SetBusy(bool bBusy, const FString& StatusText);
	
	
};
