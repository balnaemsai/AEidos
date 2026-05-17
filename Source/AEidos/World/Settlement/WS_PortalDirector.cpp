// Fill out your copyright notice in the Description page of Project Settings.


#include "World/Settlement/WS_PortalDirector.h"

#include "Core/Types/PortalTypes.h"
#include "Data/Definitions/PortalDefinitionRow.h"
#include "Data/GIS_DataRegistry.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Entities/Page/PageCharacter.h"
#include "Save/SaveGameSchema.h"
#include "World/Dungeon/WS_DungeonRuntime.h"
#include "World/Settlement/Portal/PortalActor.h"
#include "World/Settlement/WS_SettlementSpace.h"

namespace PortalDirectorKV
{
	static const TCHAR* NextPortalId = TEXT("PortalDirector.NextPortalId");
	static const TCHAR* TimeSinceLastSpawn = TEXT("PortalDirector.TimeSinceLastSpawn");
	static const TCHAR* ActivePortals = TEXT("PortalDirector.ActivePortals");
}

namespace
{
	bool IsTerminalPortalStatus(EPortalStatus Status)
	{
		return Status == EPortalStatus::Cleared
			|| Status == EPortalStatus::RaidTriggered
			|| Status == EPortalStatus::Expired;
	}

	bool ShouldAdvanceRaidTimer(const FPortalState& Portal)
	{
		return Portal.Status == EPortalStatus::Available;
	}
}

FName UWS_PortalDirector::Key_NextPortalId()
{
	return FName(PortalDirectorKV::NextPortalId);
}

FName UWS_PortalDirector::Key_TimeSinceLastSpawn()
{
	return FName(PortalDirectorKV::TimeSinceLastSpawn);
}

FName UWS_PortalDirector::Key_ActivePortals()
{
	return FName(PortalDirectorKV::ActivePortals);
}

void UWS_PortalDirector::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	ActivePortals.Reset();
	PortalActors.Reset();
	PlannedSpawnPortals.Reset();
	PlannedRemovePortals.Reset();

	NextPortalId = 1;
	TimeSinceLastSpawn = 0.f;
	bDirty = false;
}

UGIS_DataRegistry* UWS_PortalDirector::GetRegistry() const
{
	if (!GetWorld())
	{
		return nullptr;
	}

	UGameInstance* GI = GetWorld()->GetGameInstance();
	if (!GI)
	{
		return nullptr;
	}

	return GI->GetSubsystem<UGIS_DataRegistry>();
}

const FPortalDefinitionRow* UWS_PortalDirector::GetDefaultPortalDef() const
{
	UGIS_DataRegistry* Registry = GetRegistry();
	if (!Registry)
	{
		return nullptr;
	}

	return Registry->GetPortalDef(TEXT("DefaultPortal"));
}

void UWS_PortalDirector::SimPlan_Implementation(USimCommandBuffer* CommandBuffer, float FixedDeltaSeconds)
{
	PlannedSpawnPortals.Reset();
	PlannedRemovePortals.Reset();

	CheckSpawn(FixedDeltaSeconds);
	UpdatePortalTimer(FixedDeltaSeconds);
}

void UWS_PortalDirector::SimCommit_Implementation(USimCommandBuffer* CommandBuffer, float FixedDeltaSeconds)
{
	for (const FPortalState& State : PlannedSpawnPortals)
	{
		ActivePortals.Add(State.PortalId, State);
		SpawnPortalActorForState(State);
		bDirty = true;

		UE_LOG(LogTemp, Log,
			TEXT("[Portal] Spawned PortalId=%d Loc=%s"),
			State.PortalId,
			*State.Location.ToString());
	}

	FinalizeSpawnedPortals();

	for (int32 PortalId : PlannedRemovePortals)
	{
		if (FPortalState* Portal = ActivePortals.Find(PortalId))
		{
			SetPortalStatus(*Portal, EPortalStatus::Expired);
		}

		RemovePortalInternal(PortalId);
		bDirty = true;
	}
}

void UWS_PortalDirector::SimPost_Implementation(float FixedDeltaSeconds)
{
	if (bDirty)
	{
		bDirty = false;
		OnPortalListChanged.Broadcast();
	}
}

void UWS_PortalDirector::CheckSpawn(float FixedDeltaSeconds)
{
	const FPortalDefinitionRow* Def = GetDefaultPortalDef();
	if (!Def)
	{
		return;
	}

	TimeSinceLastSpawn += FixedDeltaSeconds;
	if (TimeSinceLastSpawn < Def->SpawnIntervalSeconds)
	{
		return;
	}

	TimeSinceLastSpawn = 0.f;

	const FPortalState NewState = MakePortalState(*Def);
	PlannedSpawnPortals.Add(NewState);
}

void UWS_PortalDirector::UpdatePortalTimer(float FixedDeltaSeconds)
{
	for (TPair<int32, FPortalState>& Pair : ActivePortals)
	{
		FPortalState& Portal = Pair.Value;
		NormalizePortalState(Portal);

		if (IsTerminalPortalStatus(Portal.Status))
		{
			continue;
		}

		Portal.SpawnTime += FixedDeltaSeconds;
		if (!ShouldAdvanceRaidTimer(Portal))
		{
			continue;
		}

		Portal.RaidTimer = FMath::Max(0.f, Portal.RaidTimer - FixedDeltaSeconds);
		if (Portal.RaidTimer > 0.f)
		{
			continue;
		}

		SetPortalStatus(Portal, EPortalStatus::RaidTriggered);
		UE_LOG(LogTemp, Warning,
			TEXT("[Portal] RaidTriggered PortalId=%d Tier=%d"),
			Portal.PortalId,
			Portal.Tier);

		// TODO:
		// GetWorld()->GetSubsystem<UWS_RaidDirector>()->StartRaid(...);
		PlannedRemovePortals.AddUnique(Portal.PortalId);
	}
}

void UWS_PortalDirector::FinalizeSpawnedPortals()
{
	for (const FPortalState& PlannedState : PlannedSpawnPortals)
	{
		if (FPortalState* Portal = ActivePortals.Find(PlannedState.PortalId))
		{
			SetPortalStatus(*Portal, EPortalStatus::Available);
		}
	}
}

FPortalState UWS_PortalDirector::MakePortalState(const FPortalDefinitionRow& Def)
{
	FPortalState State;
	State.PortalId = NextPortalId++;
	State.Location = ChoosePortalSpawnLocation(Def);
	State.Tier = Def.Tier;
	State.SpawnTime = 0.f;
	State.RaidTimer = Def.RaidDelaySeconds;
	State.DungeonSeed = FMath::Rand();
	State.Status = EPortalStatus::Spawning;
	State.bDungeonEntered = false;
	State.bCleared = false;
	return State;
}

FVector UWS_PortalDirector::ChoosePortalSpawnLocation(const FPortalDefinitionRow& Def) const
{
	FVector Origin = FVector::ZeroVector;

	if (UWS_SettlementSpace* SettlementSpace = GetWorld() ? GetWorld()->GetSubsystem<UWS_SettlementSpace>() : nullptr)
	{
		const TArray<FIntPoint> OwnedChunks = SettlementSpace->GetOwnedChunks();
		if (OwnedChunks.Num() > 0)
		{
			FVector Accumulated = FVector::ZeroVector;
			for (const FIntPoint& Coord : OwnedChunks)
			{
				Accumulated += SettlementSpace->GetChunkWorldLocation(Coord);
			}

			Origin = Accumulated / static_cast<float>(OwnedChunks.Num());
		}
	}

	const float Dist = FMath::FRandRange(Def.SpawnMinDistance, Def.SpawnMaxDistance);
	const float Angle = FMath::FRandRange(0.f, 2.f * PI);

	const FVector Offset(
		FMath::Cos(Angle) * Dist,
		FMath::Sin(Angle) * Dist,
		0.f);

	return Origin + Offset;
}

bool UWS_PortalDirector::SetPortalStatus(FPortalState& Portal, EPortalStatus NewStatus)
{
	if (Portal.Status == NewStatus)
	{
		NormalizePortalState(Portal);
		return false;
	}

	Portal.Status = NewStatus;
	NormalizePortalState(Portal);
	bDirty = true;
	return true;
}

void UWS_PortalDirector::NormalizePortalState(FPortalState& Portal) const
{
	switch (Portal.Status)
	{
	case EPortalStatus::Entered:
		Portal.bDungeonEntered = true;
		Portal.bCleared = false;
		break;
	case EPortalStatus::Cleared:
		Portal.bDungeonEntered = true;
		Portal.bCleared = true;
		break;
	case EPortalStatus::RaidTriggered:
	case EPortalStatus::Expired:
		Portal.bDungeonEntered = false;
		Portal.bCleared = false;
		break;
	case EPortalStatus::Spawning:
	case EPortalStatus::Available:
	default:
		Portal.bDungeonEntered = false;
		Portal.bCleared = false;
		break;
	}
}

void UWS_PortalDirector::SpawnPortalActorForState(const FPortalState& State)
{
	const FPortalDefinitionRow* Def = GetDefaultPortalDef();
	if (!Def)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Portal] No DefaultPortal def"));
		return;
	}

	if (Def->PortalActorClass.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Portal] PortalActorClass is null"));
		return;
	}

	UClass* SpawnClass = Def->PortalActorClass.LoadSynchronous();
	if (!SpawnClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Portal] Failed to load PortalActorClass"));
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AActor* Spawned = GetWorld()->SpawnActor<AActor>(
		SpawnClass,
		State.Location,
		FRotator::ZeroRotator,
		Params);

	if (!Spawned)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Portal] SpawnActor failed for PortalId=%d"), State.PortalId);
		return;
	}

	PortalActors.Add(State.PortalId, Spawned);

	if (APortalActor* PortalActor = Cast<APortalActor>(Spawned))
	{
		PortalActor->InitializePortal(State.PortalId, State.Tier);
	}
}

void UWS_PortalDirector::EnsurePortalActorsFromState()
{
	for (const TPair<int32, FPortalState>& Pair : ActivePortals)
	{
		const int32 PortalId = Pair.Key;

		if (const TWeakObjectPtr<AActor>* Found = PortalActors.Find(PortalId))
		{
			if (Found->IsValid())
			{
				continue;
			}
		}

		SpawnPortalActorForState(Pair.Value);
	}
}

void UWS_PortalDirector::RemovePortalInternal(int32 PortalId)
{
	if (TWeakObjectPtr<AActor>* Found = PortalActors.Find(PortalId))
	{
		if (Found->IsValid())
		{
			Found->Get()->Destroy();
		}
	}

	PortalActors.Remove(PortalId);
	ActivePortals.Remove(PortalId);

	UE_LOG(LogTemp, Log, TEXT("[Portal] Removed PortalId=%d"), PortalId);
}

bool UWS_PortalDirector::ValidateEntry(int32 PortalId) const
{
	const FPortalState* State = ActivePortals.Find(PortalId);
	if (!State)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Portal] ValidateEntry failed: PortalId=%d not found"), PortalId);
		return false;
	}

	if (State->Status != EPortalStatus::Available)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Portal] ValidateEntry denied: PortalId=%d Status=%d Entered=%d Cleared=%d RaidTimer=%.2f"),
			PortalId,
			static_cast<int32>(State->Status),
			State->bDungeonEntered ? 1 : 0,
			State->bCleared ? 1 : 0,
			State->RaidTimer);
		return false;
	}

	return true;
}

bool UWS_PortalDirector::RequestEnterPortal(int32 PortalId, APageCharacter* EnteringPage)
{
	if (!ValidateEntry(PortalId))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Portal] RequestEnterPortal failed PortalId=%d"), PortalId);
		return false;
	}

	if (!EnteringPage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Portal] RequestEnterPortal failed: entering page missing"));
		return false;
	}

	UWS_DungeonRuntime* DungeonRuntime = GetWorld() ? GetWorld()->GetSubsystem<UWS_DungeonRuntime>() : nullptr;
	if (!DungeonRuntime)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Portal] RequestEnterPortal failed: DungeonRuntime missing"));
		return false;
	}

	if (!DungeonRuntime->EnterDungeonForPortal(PortalId, EnteringPage))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Portal] RequestEnterPortal failed: dungeon runtime rejected request"));
		return false;
	}

	if (FPortalState* State = ActivePortals.Find(PortalId))
	{
		SetPortalStatus(*State, EPortalStatus::Entered);
	}

	UE_LOG(LogTemp, Log,
		TEXT("[Portal] RequestEnterPortal PortalId=%d PageId=%d"),
		PortalId,
		EnteringPage->GetPageEntityId());

	// TODO:
	// Actual dungeon transition/session creation hooks in here later.
	return true;
}

void UWS_PortalDirector::OnDungeonCleared(int32 PortalId)
{
	FPortalState* State = ActivePortals.Find(PortalId);
	if (!State)
	{
		return;
	}

	SetPortalStatus(*State, EPortalStatus::Cleared);

	UE_LOG(LogTemp, Log, TEXT("[Portal] DungeonCleared PortalId=%d"), PortalId);

	PlannedRemovePortals.AddUnique(PortalId);
}

void UWS_PortalDirector::SpawnPortalNow()
{
	const FPortalDefinitionRow* Def = GetDefaultPortalDef();
	if (!Def)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Portal] SpawnPortalNow failed: no default def"));
		return;
	}

	const FPortalState State = MakePortalState(*Def);
	PlannedSpawnPortals.Add(State);
}

bool UWS_PortalDirector::TryGetPortalState(int32 PortalId, FPortalState& OutState) const
{
	if (const FPortalState* Found = ActivePortals.Find(PortalId))
	{
		OutState = *Found;
		return true;
	}

	return false;
}

TArray<FPortalState> UWS_PortalDirector::GetActivePortals() const
{
	TArray<FPortalState> Result;
	Result.Reserve(ActivePortals.Num());

	for (const TPair<int32, FPortalState>& Pair : ActivePortals)
	{
		Result.Add(Pair.Value);
	}

	return Result;
}

FString UWS_PortalDirector::EncodePortalStates(const TMap<int32, FPortalState>& InStates)
{
	TArray<FString> Items;
	Items.Reserve(InStates.Num());

	for (const TPair<int32, FPortalState>& Pair : InStates)
	{
		const FPortalState& S = Pair.Value;

		Items.Add(FString::Printf(
			TEXT("%d,%.3f,%.3f,%.3f,%d,%.3f,%.3f,%d,%d,%d,%d"),
			S.PortalId,
			S.Location.X,
			S.Location.Y,
			S.Location.Z,
			S.Tier,
			S.SpawnTime,
			S.RaidTimer,
			S.DungeonSeed,
			static_cast<int32>(S.Status),
			S.bDungeonEntered ? 1 : 0,
			S.bCleared ? 1 : 0));
	}

	return FString::Join(Items, TEXT(";"));
}

void UWS_PortalDirector::DecodePortalStates(const FString& Encoded, TArray<FPortalState>& OutStates)
{
	OutStates.Reset();

	if (Encoded.IsEmpty())
	{
		return;
	}

	TArray<FString> Entries;
	Encoded.ParseIntoArray(Entries, TEXT(";"), true);

	for (const FString& Entry : Entries)
	{
		TArray<FString> Fields;
		Entry.ParseIntoArray(Fields, TEXT(","), false);

		if (Fields.Num() < 11)
		{
			continue;
		}

		FPortalState S;
		S.PortalId = FCString::Atoi(*Fields[0]);
		S.Location.X = FCString::Atof(*Fields[1]);
		S.Location.Y = FCString::Atof(*Fields[2]);
		S.Location.Z = FCString::Atof(*Fields[3]);
		S.Tier = FCString::Atoi(*Fields[4]);
		S.SpawnTime = FCString::Atof(*Fields[5]);
		S.RaidTimer = FCString::Atof(*Fields[6]);
		S.DungeonSeed = FCString::Atoi(*Fields[7]);
		S.Status = static_cast<EPortalStatus>(FCString::Atoi(*Fields[8]));
		S.bDungeonEntered = FCString::Atoi(*Fields[9]) != 0;
		S.bCleared = FCString::Atoi(*Fields[10]) != 0;

		switch (S.Status)
		{
		case EPortalStatus::Spawning:
			S.Status = EPortalStatus::Available;
			break;
		case EPortalStatus::Available:
		case EPortalStatus::Entered:
		case EPortalStatus::Cleared:
		case EPortalStatus::RaidTriggered:
		case EPortalStatus::Expired:
			break;
		default:
			if (S.bCleared)
			{
				S.Status = EPortalStatus::Cleared;
			}
			else if (S.bDungeonEntered)
			{
				S.Status = EPortalStatus::Entered;
			}
			else
			{
				S.Status = EPortalStatus::Available;
			}
			break;
		}

		OutStates.Add(S);
	}
}

void UWS_PortalDirector::WriteToSnapshot_Implementation(FEidosWorldSnapshot& OutSnapshot) const
{
	OutSnapshot.SetKVString(Key_NextPortalId(), FString::FromInt(NextPortalId));
	OutSnapshot.SetKVString(Key_TimeSinceLastSpawn(), FString::SanitizeFloat(TimeSinceLastSpawn));
	OutSnapshot.SetKVString(Key_ActivePortals(), EncodePortalStates(ActivePortals));
}

void UWS_PortalDirector::ApplySnapshot_Implementation(const FEidosWorldSnapshot& Snapshot)
{
	ActivePortals.Reset();
	PortalActors.Reset();

	NextPortalId = FCString::Atoi(*Snapshot.GetKVString(Key_NextPortalId(), TEXT("1")));
	TimeSinceLastSpawn = FCString::Atof(*Snapshot.GetKVString(Key_TimeSinceLastSpawn(), TEXT("0")));

	const FString Encoded = Snapshot.GetKVString(Key_ActivePortals(), TEXT(""));
	TArray<FPortalState> DecodedStates;
	DecodePortalStates(Encoded, DecodedStates);

	for (const FPortalState& S : DecodedStates)
	{
		FPortalState NormalizedState = S;
		NormalizePortalState(NormalizedState);

		// Dungeon runtime sessions are not persisted yet.
		// If a save is loaded after entering a portal, treat it as re-opened rather than permanently occupied.
		if (NormalizedState.Status == EPortalStatus::Entered)
		{
			NormalizedState.Status = EPortalStatus::Available;
			NormalizePortalState(NormalizedState);
		}

		ActivePortals.Add(NormalizedState.PortalId, NormalizedState);
		NextPortalId = FMath::Max(NextPortalId, NormalizedState.PortalId + 1);
	}

	EnsurePortalActorsFromState();
	bDirty = true;
}
