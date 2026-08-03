// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "UI/GIS_UIRouter.h"
#include "EidosHUD.generated.h"

class UUserWidget;
class UHUDRootWidget;

/**
 * 
 */
UCLASS()
class AEIDOS_API AEidosHUD : public AHUD
{
	GENERATED_BODY()
	
public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UHUDRootWidget* GetHUDRootWidget() const;
	
private:
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> HUDRootWidgetClass;

	UPROPERTY()
	UUserWidget* HUDRootWidgetInstance = nullptr;

	UPROPERTY()
	UGIS_UIRouter* UIRouter = nullptr;

	UFUNCTION()
	void HandleUIStateChanged(EUIState NewState);

	void EnsureHUDRoot();
	void DestroyHUDRoot();
	
};
