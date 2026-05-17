// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Save/SaveGameParticipant.h"
#include "Subsystems/WorldSubsystem.h"
#include "WS_SettlementSpace.generated.h"

class ATerritoryChunkActor;
class UEidosSaveGame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTerritoryChanged);

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
	
	UFUNCTION(BlueprintPure, Category="Territory")
	bool OwnChunk(const FIntPoint& Coord) const { return OwnedChunks.Contains(Coord); }

	UFUNCTION(BlueprintPure, Category="Territory")
	TArray<FIntPoint> GetOwnedChunks() const;

	UFUNCTION(BlueprintPure, Category="Territory")
	TArray<FIntPoint> GetExpandableChunks() const;

	UFUNCTION(BlueprintPure, Category="Territory")
	FIntPoint WorldLocationToCoord(const FVector& WorldLocation) const;

	UFUNCTION(BlueprintPure, Category="Territory")
	FVector GetChunkWorldLocation(const FIntPoint& Coord) const;

	UFUNCTION(BlueprintPure, Category="Territory")
	TSubclassOf<ATerritoryChunkActor> GetChunkActorClass() const { return ChunkActorClass; }

	UFUNCTION(BlueprintPure, Category="Territory")
	float GetChunkSizeCm() const { return ChunkSizeCm; }

	UFUNCTION(BlueprintPure, Category="Territory")
	FName GetExpansionCostResourceId() const { return ExpansionCostResourceId; }

	UFUNCTION(BlueprintPure, Category="Territory")
	int32 GetExpansionCostAmount() const { return ExpansionCostAmount; }

	UFUNCTION(BlueprintPure, Category="Territory")
	bool HasOwnedAdjacentChunk(const FIntPoint& Coord) const;

	UFUNCTION(BlueprintCallable, Category="Territory")
	bool CanPurchaseChunk(const FIntPoint& Coord, FString& OutReason) const;

	UFUNCTION(BlueprintCallable, Category="Territory")
	bool PurchaseChunk(const FIntPoint& Coord, FString& OutReason);

	UFUNCTION(BlueprintCallable, Category="Territory")
	bool PurchaseChunkAtWorldLocation(const FVector& WorldLocation, FIntPoint& OutCoord, FString& OutReason);

	UPROPERTY(BlueprintAssignable)
	FOnTerritoryChanged OnTerritoryChanged;

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

	UPROPERTY(EditDefaultsOnly, Category="Territory|Expansion")
	FName ExpansionCostResourceId = TEXT("EP");

	UPROPERTY(EditDefaultsOnly, Category="Territory|Expansion", meta=(ClampMin="0"))
	int32 ExpansionCostAmount = 100;

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
	bool IsValidExpansionTarget(const FIntPoint& Coord, FString& OutReason) const;
	bool AcquireChunkInternal(const FIntPoint& Coord, FString& OutReason, bool bConsumeCost);
	
};
