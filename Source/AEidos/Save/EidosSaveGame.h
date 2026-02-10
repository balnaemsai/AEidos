// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SaveGameSchema.h"
#include "EidosSaveGame.generated.h"

USTRUCT(BlueprintType)
struct FSettlementSpaceSaveData
{
	GENERATED_BODY()

	// 보유한 청크 좌표 목록 (ChunkX, ChunkY)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	TArray<FIntPoint> OwnedChunks;
};

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	FSettlementSpaceSaveData SettlementSpace;
	
};
