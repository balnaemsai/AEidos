// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "EidosDataRegistrySettings.generated.h"

class UEidosDataRegistryConfig;

/**
 * 
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Eidos Data Registry"))
class AEIDOS_API UEidosDataRegistrySettings : public UDeveloperSettings
{
	GENERATED_BODY()

	public:
	UPROPERTY(EditAnywhere, Config, Category = "Data Registry", meta = (AllowedClasses = "/Script/Eidos.EidosDataRegistryConfig"))
	TSoftObjectPtr<UEidosDataRegistryConfig> RegistryConfig;

	UPROPERTY(EditAnywhere, Config, Category = "Data Registry")
	bool bAllowSyncFallback = false;
	
};
