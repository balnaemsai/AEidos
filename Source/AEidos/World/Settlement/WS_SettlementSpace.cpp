// Fill out your copyright notice in the Description page of Project Settings.


#include "World/Settlement/WS_SettlementSpace.h"

#include "Engine/World.h"
#include "World/Settlement/TerritoryChunkActor.h"
#include "Save/SaveGameSchema.h"

const FName UWS_SettlementSpace::KEY_OwnedChunks(TEXT("SettlementSpace.OwnedChunks"));

void UWS_SettlementSpace::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (!ChunkActorClass)
	{
		// ✅ BP 클래스 경로 (반드시 *_C 까지)
		static const TCHAR* ChunkBPClassPath =
			TEXT("/Game/Blueprints/BP_TerritoryChunkActor.BP_TerritoryChunkActor_C");

		UClass* Loaded = LoadClass<ATerritoryChunkActor>(nullptr, ChunkBPClassPath);

		if (Loaded)
		{
			ChunkActorClass = Loaded;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[SettlementSpace] Failed to load ChunkActor BP class: %s. Fallback to native."), ChunkBPClassPath);
			ChunkActorClass = ATerritoryChunkActor::StaticClass();
		}
	}
}

void UWS_SettlementSpace::Deinitialize()
{
	OwnedChunks.Reset();
	SpawnedChunks.Reset();
	Super::Deinitialize();
}

void UWS_SettlementSpace::WriteToSnapshot_Implementation(FEidosWorldSnapshot& InOutSnapshot) const
{
	InOutSnapshot.KV.Add(KEY_OwnedChunks, EncodeOwnedChunks(OwnedChunks));
}

void UWS_SettlementSpace::ApplySnapshot_Implementation(const FEidosWorldSnapshot& Snapshot)
{
	// (KV가 TMap<FName, FString>라고 가정)
	const FString* Encoded = Snapshot.KV.Find(KEY_OwnedChunks);

	OwnedChunks.Reset();

	if (!Encoded || Encoded->IsEmpty())
	{
		// ✅ 새 세이브/값 없음 -> 원점 1청크 보장
		OwnedChunks.Add(FIntPoint(0, 0));
	}
	else
	{
		DecodeOwnedChunks(*Encoded, OwnedChunks);

		// 혹시라도 비어버리면 안전장치
		if (OwnedChunks.Num() == 0)
		{
			OwnedChunks.Add(FIntPoint(0, 0));
		}
	}

	RebuildChunkActorsFromOwned();
}

FString UWS_SettlementSpace::EncodeOwnedChunks(const TSet<FIntPoint>& Chunks)
{
	// "0,0;1,0;-1,2"
	TArray<FString> Parts;
	Parts.Reserve(Chunks.Num());

	for (const FIntPoint& P : Chunks)
	{
		Parts.Add(FString::Printf(TEXT("%d,%d"), P.X, P.Y));
	}

	return FString::Join(Parts, TEXT(";"));
}

void UWS_SettlementSpace::DecodeOwnedChunks(const FString& Encoded, TSet<FIntPoint>& OutChunks)
{
	OutChunks.Reset();

	TArray<FString> Items;
	Encoded.ParseIntoArray(Items, TEXT(";"), true);

	for (const FString& Item : Items)
	{
		FString Left, Right;
		if (!Item.Split(TEXT(","), &Left, &Right))
		{
			continue;
		}

		const int32 X = FCString::Atoi(*Left);
		const int32 Y = FCString::Atoi(*Right);

		OutChunks.Add(FIntPoint(X, Y));
	}
}

void UWS_SettlementSpace::RebuildChunkActorsFromOwned()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 1) OwnedChunks에 없는 기존 Spawned는 제거
	{
		TArray<FIntPoint> ToRemove;
		ToRemove.Reserve(SpawnedChunks.Num());

		for (const auto& Pair : SpawnedChunks)
		{
			const FIntPoint Coord = Pair.Key;
			if (!OwnedChunks.Contains(Coord))
			{
				if (ATerritoryChunkActor* A = Pair.Value.Get())
				{
					A->Destroy();
				}
				ToRemove.Add(Coord);
			}
		}

		for (const FIntPoint& C : ToRemove)
		{
			SpawnedChunks.Remove(C);
		}
	}

	// 2) OwnedChunks에 있는데 아직 Spawned가 없으면 스폰
	for (const FIntPoint& Coord : OwnedChunks)
	{
		if (SpawnedChunks.Contains(Coord) && SpawnedChunks[Coord].IsValid())
		{
			continue;
		}
		SpawnChunkActor(Coord);
	}
}

FVector UWS_SettlementSpace::CoordToWorldLocation(const FIntPoint& Coord) const
{
	// 청크 중앙 기준 배치: (X*Size, Y*Size, Z)
	return FVector(Coord.X * ChunkSizeCm, Coord.Y * ChunkSizeCm, ChunkZ);
}

ATerritoryChunkActor* UWS_SettlementSpace::SpawnChunkActor(const FIntPoint& Coord)
{
	UWorld* World = GetWorld();
	if (!World || !ChunkActorClass)
	{
		return nullptr;
	}

	const FVector Loc = CoordToWorldLocation(Coord);
	const FTransform TM(FRotator::ZeroRotator, Loc);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ATerritoryChunkActor* Chunk = World->SpawnActor<ATerritoryChunkActor>(ChunkActorClass, TM, Params);
	if (Chunk)
	{
		Chunk->InitChunk(Coord, ChunkSizeCm);
		SpawnedChunks.Add(Coord, Chunk);
	}

	return Chunk;
}
