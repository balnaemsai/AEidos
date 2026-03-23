// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SaveGameSchema.h"
#include "EidosSaveGame.generated.h"


/**
 * 
 */
UCLASS()
class AEIDOS_API UEidosSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FEidosWorldSnapshot Snapshot;
	
};
