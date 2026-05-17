// Fill out your copyright notice in the Description page of Project Settings.


#include "World/Settlement/WS_SettlementSpace.h"

#include "Engine/World.h"
#include "Save/SaveGameSchema.h"
#include "World/Settlement/TerritoryChunkActor.h"
#include "World/Settlement/WS_Economy.h"

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
	OnTerritoryChanged.Broadcast();
}

TArray<FIntPoint> UWS_SettlementSpace::GetOwnedChunks() const
{
	TArray<FIntPoint> Result = OwnedChunks.Array();
	Result.Sort([](const FIntPoint& A, const FIntPoint& B)
	{
		return (A.X == B.X) ? (A.Y < B.Y) : (A.X < B.X);
	});
	return Result;
}

TArray<FIntPoint> UWS_SettlementSpace::GetExpandableChunks() const
{
	static const FIntPoint Offsets[] =
	{
		FIntPoint(1, 0),
		FIntPoint(-1, 0),
		FIntPoint(0, 1),
		FIntPoint(0, -1)
	};

	TSet<FIntPoint> CandidateSet;
	for (const FIntPoint& Owned : OwnedChunks)
	{
		for (const FIntPoint& Offset : Offsets)
		{
			const FIntPoint Candidate = Owned + Offset;
			FString Reason;
			if (IsValidExpansionTarget(Candidate, Reason))
			{
				CandidateSet.Add(Candidate);
			}
		}
	}

	TArray<FIntPoint> Result = CandidateSet.Array();
	Result.Sort([](const FIntPoint& A, const FIntPoint& B)
	{
		return (A.X == B.X) ? (A.Y < B.Y) : (A.X < B.X);
	});
	return Result;
}

FIntPoint UWS_SettlementSpace::WorldLocationToCoord(const FVector& WorldLocation) const
{
	if (ChunkSizeCm <= KINDA_SMALL_NUMBER)
	{
		return FIntPoint::ZeroValue;
	}

	return FIntPoint(
		FMath::RoundToInt(WorldLocation.X / ChunkSizeCm),
		FMath::RoundToInt(WorldLocation.Y / ChunkSizeCm));
}

FVector UWS_SettlementSpace::GetChunkWorldLocation(const FIntPoint& Coord) const
{
	return CoordToWorldLocation(Coord);
}

bool UWS_SettlementSpace::HasOwnedAdjacentChunk(const FIntPoint& Coord) const
{
	static const FIntPoint Offsets[] =
	{
		FIntPoint(1, 0),
		FIntPoint(-1, 0),
		FIntPoint(0, 1),
		FIntPoint(0, -1)
	};

	for (const FIntPoint& Offset : Offsets)
	{
		if (OwnedChunks.Contains(Coord + Offset))
		{
			return true;
		}
	}

	return false;
}

bool UWS_SettlementSpace::CanPurchaseChunk(const FIntPoint& Coord, FString& OutReason) const
{
	return IsValidExpansionTarget(Coord, OutReason);
}

bool UWS_SettlementSpace::PurchaseChunk(const FIntPoint& Coord, FString& OutReason)
{
	return AcquireChunkInternal(Coord, OutReason, true);
}

bool UWS_SettlementSpace::PurchaseChunkAtWorldLocation(const FVector& WorldLocation, FIntPoint& OutCoord, FString& OutReason)
{
	OutCoord = WorldLocationToCoord(WorldLocation);
	return PurchaseChunk(OutCoord, OutReason);
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

bool UWS_SettlementSpace::IsValidExpansionTarget(const FIntPoint& Coord, FString& OutReason) const
{
	if (OwnedChunks.Contains(Coord))
	{
		OutReason = TEXT("Chunk is already owned.");
		return false;
	}

	if (OwnedChunks.Num() > 0 && !HasOwnedAdjacentChunk(Coord))
	{
		OutReason = TEXT("Chunk must be adjacent to owned territory.");
		return false;
	}

	OutReason.Reset();
	return true;
}

bool UWS_SettlementSpace::AcquireChunkInternal(const FIntPoint& Coord, FString& OutReason, bool bConsumeCost)
{
	if (!IsValidExpansionTarget(Coord, OutReason))
	{
		return false;
	}

	if (bConsumeCost)
	{
		UWS_Economy* Economy = GetWorld() ? GetWorld()->GetSubsystem<UWS_Economy>() : nullptr;
		if (!Economy)
		{
			OutReason = TEXT("Economy subsystem is missing.");
			return false;
		}

		if (ExpansionCostAmount > 0)
		{
			if (ExpansionCostResourceId.IsNone())
			{
				OutReason = TEXT("Expansion cost resource is not configured.");
				return false;
			}

			const int32 CurrentAmount = Economy->GetAmount(ExpansionCostResourceId);
			if (CurrentAmount < ExpansionCostAmount)
			{
				OutReason = FString::Printf(
					TEXT("Need %s x%d (Have %d)"),
					*ExpansionCostResourceId.ToString(),
					ExpansionCostAmount,
					CurrentAmount);
				return false;
			}

			Economy->AddAmount(ExpansionCostResourceId, -ExpansionCostAmount);
		}
	}

	OwnedChunks.Add(Coord);
	SpawnChunkActor(Coord);
	OnTerritoryChanged.Broadcast();

	UE_LOG(LogTemp, Log,
		TEXT("[SettlementSpace] %s chunk (%d,%d)"),
		bConsumeCost ? TEXT("Purchased") : TEXT("Acquired"),
		Coord.X,
		Coord.Y);

	OutReason.Reset();
	return true;
}
