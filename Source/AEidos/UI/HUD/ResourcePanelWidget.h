// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ResourcePanelWidget.generated.h"

class UTextBlock;
class UButton;

/**
 * 
 */
UCLASS()
class AEIDOS_API UResourcePanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetResources(int32 Food, int32 Wood, int32 Stone, int32 Metal, int32 EP);

protected:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* TxtFood;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TxtWood;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TxtStone;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TxtMetal;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TxtEP;
	
};
