// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TopBarWidget.generated.h"

class UTextBlock;
class UButton;

/**
 * 
 */
UCLASS()
class AEIDOS_API UTopBarWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* TxtDateTime;

	UPROPERTY(meta = (BindWidget))
	UButton* BtnSpeed1;

	UPROPERTY(meta = (BindWidget))
	UButton* BtnSpeed2;

	UPROPERTY(meta = (BindWidget))
	UButton* BtnSpeed4;
	
};
