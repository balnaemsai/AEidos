// Fill out your copyright notice in the Description page of Project Settings.


#include "Save/GIS_SaveLoad.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Save/EidosSaveGame.h"
#include "Save/SaveGameParticipant.h"
#include "Simulation/SimSystem.h"
#include "Subsystems/WorldSubsystem.h"
#include "World/Dungeon/WS_DungeonRuntime.h"
#include "World/Settlement/WS_RaidDirector.h"
#include "UObject/UObjectIterator.h"

DEFINE_LOG_CATEGORY(LogSaveLoad);

void UGIS_SaveLoad::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	bHasPendingSnapshot = false;
	bHasNewGameSnapshot = false;
	PendingSnapshot = FEidosWorldSnapshot{};
	NewGameSnapshot = FEidosWorldSnapshot{};
}

void UGIS_SaveLoad::Deinitialize()
{
	bHasPendingSnapshot = false;
	bHasNewGameSnapshot = false;
	Super::Deinitialize();
}

void UGIS_SaveLoad::SetPendingSnapshot(const FEidosWorldSnapshot& InSnapshot)
{
	PendingSnapshot = InSnapshot;
	bHasPendingSnapshot = true;
}

void UGIS_SaveLoad::ClearPendingSnapshot()
{
	PendingSnapshot = FEidosWorldSnapshot{};
	bHasPendingSnapshot = false;
}

void UGIS_SaveLoad::StartNewGame(const FString& MapNameHint)
{
	ClearPendingSnapshot();
	bHasNewGameSnapshot = false;
	NewGameSnapshot = FEidosWorldSnapshot{};
	BuildNewGameSnapshotIfNeeded(MapNameHint);
}

void UGIS_SaveLoad::BuildNewGameSnapshotIfNeeded(const FString& MapNameHint)
{
	if (bHasNewGameSnapshot)
	{
		return;
	}

	FEidosWorldSnapshot Snapshot;
	Snapshot.SchemaVersion = 1;
	Snapshot.MapName = MapNameHint;
	Snapshot.SavedAtUtc = FDateTime::UtcNow();
	Snapshot.Game.Mode = TEXT("NewGame");
	Snapshot.Game.WorldSeed = 12345;
	Snapshot.Economy.ResourceAmounts.Add(TEXT("EP"), 500);

	NewGameSnapshot = Snapshot;
	bHasNewGameSnapshot = true;
}

void UGIS_SaveLoad::ApplyPendingSnapshotToWorld(UWorld& World)
{
	if (bHasPendingSnapshot)
	{
		DispatchApplySnapshot(World, PendingSnapshot);
		ClearPendingSnapshot();
		return;
	}

	if (!bHasNewGameSnapshot)
	{
		BuildNewGameSnapshotIfNeeded(World.GetMapName());
	}

	DispatchApplySnapshot(World, NewGameSnapshot);
}

bool UGIS_SaveLoad::SaveToSlot(UWorld& World, const FString& SlotName, int32 UserIndex)
{
	FString FailureReason;
	if (!CanSaveWorld(World, FailureReason))
	{
		UE_LOG(LogSaveLoad, Warning, TEXT("Save rejected: %s"), *FailureReason);
		return false;
	}

	UEidosSaveGame* SaveObj = Cast<UEidosSaveGame>(UGameplayStatics::CreateSaveGameObject(UEidosSaveGame::StaticClass()));
	if (!SaveObj)
	{
		return false;
	}

	SaveObj->Snapshot = CaptureWorldSnapshot(World);
	return UGameplayStatics::SaveGameToSlot(SaveObj, SlotName, UserIndex);
}

bool UGIS_SaveLoad::CanSaveWorld(const UWorld& World, FString& OutReason) const
{
	OutReason.Reset();

	if (const UWS_DungeonRuntime* DungeonRuntime = World.GetSubsystem<UWS_DungeonRuntime>())
	{
		if (DungeonRuntime->HasActiveDungeon())
		{
			OutReason = TEXT("An active dungeon expedition is not yet supported by the save format.");
			return false;
		}
	}

	if (const UWS_RaidDirector* RaidDirector = World.GetSubsystem<UWS_RaidDirector>())
	{
		if (RaidDirector->HasAnyActiveRaid())
		{
			OutReason = TEXT("An active raid is not yet supported by the save format.");
			return false;
		}
	}

	return true;
}

bool UGIS_SaveLoad::LoadFromSlotToPending(const FString& SlotName, int32 UserIndex)
{
	if (!UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
	{
		return false;
	}

	UEidosSaveGame* LoadedObj = Cast<UEidosSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex));
	if (!LoadedObj)
	{
		return false;
	}

	SetPendingSnapshot(LoadedObj->Snapshot);
	return true;
}

void UGIS_SaveLoad::GatherSaveParticipants(UWorld& World, TArray<UObject*>& OutParticipants) const
{
	OutParticipants.Reset();

	for (TObjectIterator<UWorldSubsystem> It; It; ++It)
	{
		UWorldSubsystem* Subsystem = *It;
		if (!IsValid(Subsystem) || Subsystem->HasAnyFlags(RF_ClassDefaultObject))
		{
			continue;
		}
		if (Subsystem->GetWorld() != &World)
		{
			continue;
		}
		if (!Subsystem->GetClass()->ImplementsInterface(USaveGameParticipant::StaticClass()))
		{
			continue;
		}

		OutParticipants.Add(Subsystem);
	}

	for (TActorIterator<AActor> It(&World); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor))
		{
			continue;
		}
		if (Actor->GetClass()->ImplementsInterface(USaveGameParticipant::StaticClass()))
		{
			OutParticipants.Add(Actor);
		}
	}

	OutParticipants.Sort([](const UObject& A, const UObject& B)
	{
		const bool bAIsActor = A.IsA<AActor>();
		const bool bBIsActor = B.IsA<AActor>();
		if (bAIsActor != bBIsActor)
		{
			return !bAIsActor;
		}

		const int32 OrderA = A.GetClass()->ImplementsInterface(USimSystem::StaticClass()) ? ISimSystem::Execute_GetSimOrder(const_cast<UObject*>(&A)) : 0;
		const int32 OrderB = B.GetClass()->ImplementsInterface(USimSystem::StaticClass()) ? ISimSystem::Execute_GetSimOrder(const_cast<UObject*>(&B)) : 0;
		if (OrderA != OrderB)
		{
			return OrderA < OrderB;
		}

		const FString ClassA = A.GetClass()->GetName();
		const FString ClassB = B.GetClass()->GetName();
		if (ClassA != ClassB)
		{
			return ClassA < ClassB;
		}

		return A.GetName() < B.GetName();
	});
}

FEidosWorldSnapshot UGIS_SaveLoad::CaptureWorldSnapshot(UWorld& World) const
{
	FEidosWorldSnapshot Snapshot;
	Snapshot.SchemaVersion = 1;
	Snapshot.MapName = World.GetMapName();
	Snapshot.SavedAtUtc = FDateTime::UtcNow();

	TArray<UObject*> Participants;
	GatherSaveParticipants(World, Participants);
	for (UObject* Participant : Participants)
	{
		ISaveGameParticipant::Execute_WriteToSnapshot(Participant, Snapshot);
	}

	return Snapshot;
}

void UGIS_SaveLoad::DispatchApplySnapshot(UWorld& World, const FEidosWorldSnapshot& Snapshot)
{
	TArray<UObject*> Participants;
	GatherSaveParticipants(World, Participants);
	for (UObject* Participant : Participants)
	{
		ISaveGameParticipant::Execute_ApplySnapshot(Participant, Snapshot);
	}
}
