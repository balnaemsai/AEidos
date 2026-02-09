// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/HUD/Panels/PanelLifecycle.h"
#include "Panel_Recruit.generated.h"

/**
 * 
 */
UCLASS()
class AEIDOS_API UPanel_Recruit : public UUserWidget, public IPanelLifecycle
{
	GENERATED_BODY()

public:
	virtual void OnPanelShown_Implementation() override;
	virtual void OnPanelHidden_Implementation() override;
	
};
