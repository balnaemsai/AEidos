// Fill out your copyright notice in the Description page of Project Settings.


#include "Save/GIS_SaveLoad.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Save/EidosSaveGame.h"
#include "Save/SaveGameParticipant.h"
#include "Simulation/SimSystem.h"
#include "Subsystems/WorldSubsystem.h"
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
	UEidosSaveGame* SaveObj = Cast<UEidosSaveGame>(UGameplayStatics::CreateSaveGameObject(UEidosSaveGame::StaticClass()));
	if (!SaveObj)
	{
		return false;
	}

	SaveObj->Snapshot = CaptureWorldSnapshot(World);
	return UGameplayStatics::SaveGameToSlot(SaveObj, SlotName, UserIndex);
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

	OutParticipants.Sort([](UObject* const& A, UObject* const& B)
	{
		const bool bAIsActor = A && A->IsA<AActor>();
		const bool bBIsActor = B && B->IsA<AActor>();
		if (bAIsActor != bBIsActor)
		{
			return !bAIsActor;
		}

		const int32 OrderA = (A && A->GetClass()->ImplementsInterface(USimSystem::StaticClass())) ? ISimSystem::Execute_GetSimOrder(A) : 0;
		const int32 OrderB = (B && B->GetClass()->ImplementsInterface(USimSystem::StaticClass())) ? ISimSystem::Execute_GetSimOrder(B) : 0;
		if (OrderA != OrderB)
		{
			return OrderA < OrderB;
		}

		const FString ClassA = A ? A->GetClass()->GetName() : FString();
		const FString ClassB = B ? B->GetClass()->GetName() : FString();
		if (ClassA != ClassB)
		{
			return ClassA < ClassB;
		}

		return A && B ? A->GetName() < B->GetName() : A != nullptr;
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
