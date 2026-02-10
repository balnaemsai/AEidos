// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Save/SaveGameParticipant.h"
#include "Subsystems/WorldSubsystem.h"
#include "WS_SettlementSpace.generated.h"

class ATerritoryChunkActor;
class UEidosSaveGame;

/**
 * 
 */
UCLASS()
class AEIDOS_API UWS_SettlementSpace : public UWorldSubsystem, public ISaveGameParticipant
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	// --- Load/Save 연결용 API ---
	void LoadFromSave(const UEidosSaveGame* Save);
	void WriteToSave(UEidosSaveGame* Save) const;

	virtual void WriteToSnapshot_Implementation(FEidosWorldSnapshot& InOutSnapshot) const override;
	virtual void ApplySnapshot_Implementation(const FEidosWorldSnapshot& Snapshot) override;
	
	bool OwnChunk(const FIntPoint& Coord) const { return OwnedChunks.Contains(Coord); }

private:
	static FString EncodeOwnedChunks(const TSet<FIntPoint>& Chunks);
	static void DecodeOwnedChunks(const FString& Encoded, TSet<FIntPoint>& OutChunks);

	// 적용 후 월드에 실제 액터 반영
	void RebuildChunkActorsFromOwned();

	ATerritoryChunkActor* SpawnChunkActor(const FIntPoint& Coord);
	FVector CoordToWorldLocation(const FIntPoint& Coord) const;

	// 10m x 10m = 1000cm
	UPROPERTY(EditDefaultsOnly, Category="Territory")
	float ChunkSizeCm = 1000.f;

	UPROPERTY(EditDefaultsOnly, Category="Territory")
	float ChunkZ = 0.f;

	// 스폰 클래스(원하면 BP로 교체 가능)
	UPROPERTY(EditDefaultsOnly, Category="Territory")
	TSubclassOf<ATerritoryChunkActor> ChunkActorClass;

	// 보유 목록(저장 대상)
	UPROPERTY()
	TSet<FIntPoint> OwnedChunks;

	// 스폰된 액터 캐시
	UPROPERTY()
	TMap<FIntPoint, TWeakObjectPtr<ATerritoryChunkActor>> SpawnedChunks;

	static const FName KEY_OwnedChunks;
	
};
