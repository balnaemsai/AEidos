// Fill out your copyright notice in the Description page of Project Settings.


#include "World/Settlement/WS_PortalDirector.h"

#include "Core/Types/PortalTypes.h"
#include "Data/Definitions/PortalDefinitionRow.h"
#include "Data/Definitions/DungeonAttributeDefinitionRow.h"
#include "Data/GIS_DataRegistry.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Entities/Page/PageCharacter.h"
#include "Save/SaveGameSchema.h"
#include "World/Dungeon/WS_DungeonRuntime.h"
#include "World/Settlement/Portal/PortalActor.h"
#include "World/Settlement/WS_RaidDirector.h"
#include "World/Settlement/WS_Population.h"
#include "World/Settlement/WS_SettlementSpace.h"
#include "World/Settlement/WS_SettlementValue.h"

namespace PortalDirectorKV
{
	static const TCHAR* NextPortalId = TEXT("PortalDirector.NextPortalId");
	static const TCHAR* TimeSinceLastSpawn = TEXT("PortalDirector.TimeSinceLastSpawn");
	static const TCHAR* SpawnTimers = TEXT("PortalDirector.SpawnTimers");
	static const TCHAR* ActivePortals = TEXT("PortalDirector.ActivePortals");
}

namespace
{
	FString EncodeDungeonAttributes(const TArray<FDungeonAttributeWeight>& Attributes)
	{
		TArray<FString> Parts;
		for (const FDungeonAttributeWeight& Attribute : Attributes)
		{
			if (!Attribute.AttributeId.IsNone() && Attribute.Weight > 0.f && Attribute.Strength > 0)
			{
				Parts.Add(FString::Printf(TEXT("%s=%.4f,%d"), *Attribute.AttributeId.ToString(), Attribute.Weight, Attribute.Strength));
			}
		}
		return FString::Join(Parts, TEXT("|"));
	}

	void DecodeDungeonAttributes(const FString& Encoded, TArray<FDungeonAttributeWeight>& OutAttributes)
	{
		OutAttributes.Reset();
		TArray<FString> Parts;
		Encoded.ParseIntoArray(Parts, TEXT("|"), true);
		for (const FString& Part : Parts)
		{
			FString Id, Values;
			if (Part.Split(TEXT("="), &Id, &Values) && !Id.IsEmpty())
			{
				TArray<FString> ValueFields;
				Values.ParseIntoArray(ValueFields, TEXT(","), false);
				const float ParsedWeight = ValueFields.Num() > 0 ? FCString::Atof(*ValueFields[0]) : 0.f;
				// V3 saves only contain composition weight. Keep them usable, but new portals
				// persist the actual altar strength in the second field.
				const int32 ParsedStrength = ValueFields.Num() > 1 ? FCString::Atoi(*ValueFields[1]) : FMath::Max(1, FMath::RoundToInt(ParsedWeight * 10.f));
				if (ParsedWeight > 0.f && ParsedStrength > 0) OutAttributes.Add({ FName(*Id), ParsedWeight, ParsedStrength });
			}
		}
	}

	bool IsTerminalPortalStatus(EPortalStatus Status)
	{
		return Status == EPortalStatus::Cleared
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

FName UWS_PortalDirector::Key_SpawnTimers()
{
	return FName(PortalDirectorKV::SpawnTimers);
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
	TimeSinceLastSpawnByDef.Reset();
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

const FPortalDefinitionRow* UWS_PortalDirector::GetPortalDef(FName PortalDefId) const
{
	UGIS_DataRegistry* Registry = GetRegistry();
	if (!Registry || PortalDefId.IsNone())
	{
		return nullptr;
	}

	return Registry->GetPortalDef(PortalDefId);
}

TArray<FName> UWS_PortalDirector::GetAutoSpawnPortalDefIds() const
{
	TArray<FName> Result;

	UGIS_DataRegistry* Registry = GetRegistry();
	if (!Registry)
	{
		return Result;
	}

	Result = Registry->GetAllPortalDefIds();
	Result.RemoveAll([Registry](const FName PortalDefId)
	{
		const FPortalDefinitionRow* Def = Registry->GetPortalDef(PortalDefId);
		return !Def || !Def->bEnableAutoSpawn;
	});

	Result.Sort([](const FName& A, const FName& B)
	{
		if (A == TEXT("TestDungeon"))
		{
			return true;
		}
		if (B == TEXT("TestDungeon"))
		{
			return false;
		}
		if (A == TEXT("DefaultPortal"))
		{
			return true;
		}
		if (B == TEXT("DefaultPortal"))
		{
			return false;
		}
		return A.LexicalLess(B);
	});

	return Result;
}

int32 UWS_PortalDirector::GetActivePortalCountForDef(FName PortalDefId) const
{
	int32 Count = 0;
	for (const TPair<int32, FPortalState>& Pair : ActivePortals)
	{
		if (Pair.Value.PortalDefId == PortalDefId && !IsTerminalPortalStatus(Pair.Value.Status))
		{
			++Count;
		}
	}
	return Count;
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

	// RaidTriggered portals remain in the world while their spawned wave is active.
	// StartRaid de-duplicates per portal, so this is safe across later fixed ticks.
	if (UWS_RaidDirector* RaidDirector = GetWorld() ? GetWorld()->GetSubsystem<UWS_RaidDirector>() : nullptr)
	{
		for (const TPair<int32, FPortalState>& Pair : ActivePortals)
		{
			if (Pair.Value.Status == EPortalStatus::RaidTriggered)
			{
				RaidDirector->StartRaid(Pair.Value);
			}
		}
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
	const TArray<FName> PortalDefIds = GetAutoSpawnPortalDefIds();
	for (const FName PortalDefId : PortalDefIds)
	{
		const FPortalDefinitionRow* Def = GetPortalDef(PortalDefId);
		if (!Def)
		{
			continue;
		}

		float& SpawnTimer = TimeSinceLastSpawnByDef.FindOrAdd(PortalDefId);
		if (PortalDefId == TEXT("DefaultPortal") && SpawnTimer <= 0.f && TimeSinceLastSpawn > 0.f)
		{
			SpawnTimer = TimeSinceLastSpawn;
		}

		SpawnTimer += FixedDeltaSeconds;
		if (SpawnTimer < Def->SpawnIntervalSeconds)
		{
			continue;
		}

		if (Def->MaxActiveCount > 0 && GetActivePortalCountForDef(PortalDefId) >= Def->MaxActiveCount)
		{
			continue;
		}

		SpawnTimer = 0.f;
		if (PortalDefId == TEXT("DefaultPortal"))
		{
			TimeSinceLastSpawn = 0.f;
		}

		const FPortalState NewState = MakePortalState(*Def);
		PlannedSpawnPortals.Add(NewState);
	}
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
			TEXT("[Portal] RaidTriggered PortalId=%d Difficulty=%.2f"),
			Portal.PortalId,
			Portal.DungeonDifficulty);

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
	State.PortalDefId = Def.PortalId.IsNone() ? TEXT("DefaultPortal") : Def.PortalId;
	State.Location = ChoosePortalSpawnLocation(Def);
	State.DungeonSeed = FMath::Rand();
	State.SettlementValueAtSpawn = GetWorld() ? GetWorld()->GetSubsystem<UWS_SettlementValue>()->GetCurrentSettlementValue() : 0.f;
	State.DungeonDifficulty = GetWorld()
		? GetWorld()->GetSubsystem<UWS_SettlementValue>()->RollDungeonDifficulty(State.DungeonSeed)
		: 1.f;
	RollDungeonAttributes(State);
	State.SpawnTime = 0.f;
	State.RaidTimer = Def.RaidDelaySeconds;
	State.Status = EPortalStatus::Spawning;
	State.bDungeonEntered = false;
	State.bCleared = false;
	return State;
}

void UWS_PortalDirector::RollDungeonAttributes(FPortalState& InOutState) const
{
	UGIS_DataRegistry* Registry = GetRegistry();
	if (!Registry || !Registry->EnsureReadySync()) return;

	const float TargetDifficulty = FMath::Max(0.5f, InOutState.DungeonDifficulty);
	TArray<const FDungeonAttributeDefinitionRow*> Candidates;
	Registry->GetEligibleDungeonAttributes(TargetDifficulty, Candidates);
	Candidates.RemoveAll([Registry, TargetDifficulty](const FDungeonAttributeDefinitionRow* Row)
	{
		return !Row || !Registry->GetItemDef(Row->CoreShardItemId)
			|| Row->DifficultyWeight <= 0.f || Row->MinimumStrength <= 0
			|| Row->DifficultyWeight * Row->MinimumStrength > TargetDifficulty;
	});
	if (Candidates.IsEmpty()) return;

	FRandomStream Random(InOutState.DungeonSeed);
	const int32 TargetCount = FMath::Min(Candidates.Num(), TargetDifficulty >= 5.f ? 3 : TargetDifficulty >= 2.f ? 2 : 1);
	float RemainingDifficulty = TargetDifficulty;
	for (int32 Index = 0; Index < TargetCount && !Candidates.IsEmpty(); ++Index)
	{
		float CandidateWeight = 0.f;
		for (const FDungeonAttributeDefinitionRow* Candidate : Candidates)
		{
			if (Candidate->DifficultyWeight * Candidate->MinimumStrength <= RemainingDifficulty)
			{
				CandidateWeight += Candidate->SelectionWeight;
			}
		}
		if (CandidateWeight <= 0.f) break;
		float Roll = Random.FRandRange(0.f, CandidateWeight);
		const FDungeonAttributeDefinitionRow* Selected = nullptr;
		for (const FDungeonAttributeDefinitionRow* Candidate : Candidates)
		{
			if (Candidate->DifficultyWeight * Candidate->MinimumStrength > RemainingDifficulty) continue;
			Roll -= Candidate->SelectionWeight;
			if (Roll <= 0.f) { Selected = Candidate; break; }
		}
		if (!Selected) break;
		const int32 MaxByDifficulty = FMath::FloorToInt(RemainingDifficulty / Selected->DifficultyWeight);
		const int32 MaxStrength = FMath::Max(Selected->MinimumStrength, FMath::Min(Selected->MaximumStrength, MaxByDifficulty));
		const int32 Strength = Random.RandRange(Selected->MinimumStrength, MaxStrength);
		InOutState.DungeonAttributes.Add({ Selected->AttributeId, 0.f, Strength });
		RemainingDifficulty -= Selected->DifficultyWeight * Strength;
		Candidates.Remove(Selected);
	}

	float TotalStrength = 0.f;
	float FinalDifficulty = 0.f;
	for (const FDungeonAttributeWeight& Attribute : InOutState.DungeonAttributes)
	{
		TotalStrength += Attribute.Strength;
		if (const FDungeonAttributeDefinitionRow* Definition = Registry->GetDungeonAttributeDef(Attribute.AttributeId))
		{
			FinalDifficulty += Definition->DifficultyWeight * Attribute.Strength;
		}
	}
	for (FDungeonAttributeWeight& Attribute : InOutState.DungeonAttributes)
	{
		Attribute.Weight = TotalStrength > 0.f ? Attribute.Strength / TotalStrength : 0.f;
	}
	// Composition is authoritative: combat, raids, map generation, and the shard
	// all use this same weighted sum rather than a separate difficulty estimate.
	InOutState.DungeonDifficulty = FMath::Max(0.5f, FinalDifficulty);
}

FVector UWS_PortalDirector::ChoosePortalSpawnLocation(const FPortalDefinitionRow& Def) const
{
	if (UWS_SettlementSpace* SettlementSpace = GetWorld() ? GetWorld()->GetSubsystem<UWS_SettlementSpace>() : nullptr)
	{
		const TArray<FIntPoint> OwnedChunks = SettlementSpace->GetOwnedChunks();
		if (!OwnedChunks.IsEmpty())
		{
			static const FIntPoint Directions[] =
			{
				FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1)
			};

			// A portal must always stand on purchased ground. Prefer outer chunks so
			// it still reads as an incoming threat instead of appearing at the core.
			TArray<FIntPoint> BoundaryChunks;
			for (const FIntPoint& Coord : OwnedChunks)
			{
				for (const FIntPoint& Direction : Directions)
				{
					if (!SettlementSpace->OwnChunk(Coord + Direction))
					{
						BoundaryChunks.Add(Coord);
						break;
					}
				}
			}

			const TArray<FIntPoint>& Candidates = BoundaryChunks.IsEmpty() ? OwnedChunks : BoundaryChunks;
			const FIntPoint ChosenChunk = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
			TArray<FIntPoint> OpenDirections;
			for (const FIntPoint& Direction : Directions)
			{
				if (!SettlementSpace->OwnChunk(ChosenChunk + Direction))
				{
					OpenDirections.Add(Direction);
				}
			}

			const FIntPoint EdgeDirection = OpenDirections.IsEmpty()
				? Directions[FMath::RandRange(0, UE_ARRAY_COUNT(Directions) - 1)]
				: OpenDirections[FMath::RandRange(0, OpenDirections.Num() - 1)];
			const FVector Direction(static_cast<float>(EdgeDirection.X), static_cast<float>(EdgeDirection.Y), 0.f);
			const FVector Tangent(-Direction.Y, Direction.X, 0.f);
			const float ChunkSize = SettlementSpace->GetChunkSizeCm();
			const FVector Offset = Direction * (ChunkSize * 0.28f) + Tangent * FMath::FRandRange(-ChunkSize * 0.16f, ChunkSize * 0.16f);
			return SettlementSpace->GetChunkWorldLocation(ChosenChunk) + Offset;
		}
	}

	// This only applies before settlement space has initialized.
	return FVector::ZeroVector;
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
	const FName PortalDefId = State.PortalDefId.IsNone() ? FName(TEXT("DefaultPortal")) : State.PortalDefId;
	const FPortalDefinitionRow* Def = GetPortalDef(PortalDefId);
	if (!Def)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Portal] Missing portal def for PortalId=%d DefId=%s"), State.PortalId, *PortalDefId.ToString());
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
		PortalActor->InitializePortal(State.PortalId, State.DungeonDifficulty);
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

	// An entered portal remains usable while its dungeon is intact so additional
	// Pages can join the same expedition before the core is destroyed.
	const bool bJoiningActiveDungeon = DungeonRuntime->IsActiveDungeonForPortal(PortalId);
	if (!bJoiningActiveDungeon && !ValidateEntry(PortalId))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Portal] RequestEnterPortal failed PortalId=%d"), PortalId);
		return false;
	}

	TArray<APageCharacter*> ExpeditionPages;
	if (UWS_Population* Population = GetWorld()->GetSubsystem<UWS_Population>();
		Population && Population->IsPageInExpeditionRoster(EnteringPage->GetPageEntityId()))
	{
		Population->GetReadyExpeditionPages(ExpeditionPages);
	}
	if (!ExpeditionPages.Contains(EnteringPage))
	{
		ExpeditionPages.Add(EnteringPage);
	}

	if (!DungeonRuntime->EnterDungeonForPortal(PortalId, ExpeditionPages))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Portal] RequestEnterPortal failed: dungeon runtime rejected request"));
		return false;
	}

	if (!bJoiningActiveDungeon)
	{
		if (FPortalState* State = ActivePortals.Find(PortalId))
		{
			SetPortalStatus(*State, EPortalStatus::Entered);
		}
	}

	UE_LOG(LogTemp, Log,
		TEXT("[Portal] RequestEnterPortal PortalId=%d PageId=%d ExpeditionPages=%d"),
		PortalId,
		EnteringPage->GetPageEntityId(),
		ExpeditionPages.Num());

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

	// This can be called during another system's commit phase.  Queueing it for
	// the next portal plan is unreliable because that plan resets its queue.
	// Remove both the visible portal and its active state immediately on clear.
	RemovePortalInternal(PortalId);
	bDirty = true;
}

void UWS_PortalDirector::ResolveRaid(int32 PortalId)
{
	FPortalState* State = ActivePortals.Find(PortalId);
	if (!State || State->Status != EPortalStatus::RaidTriggered)
	{
		return;
	}

	SetPortalStatus(*State, EPortalStatus::Expired);
	RemovePortalInternal(PortalId);
	bDirty = true;
}

void UWS_PortalDirector::FailRaid(int32 PortalId)
{
	FPortalState* State = ActivePortals.Find(PortalId);
	if (!State || State->Status != EPortalStatus::RaidTriggered)
	{
		return;
	}

	SetPortalStatus(*State, EPortalStatus::Expired);
	RemovePortalInternal(PortalId);
	bDirty = true;
	UE_LOG(LogTemp, Warning, TEXT("[Portal] Raid failed because the settlement core was destroyed. PortalId=%d"), PortalId);
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
			TEXT("V3,%d,%s,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%d,%d,%d,%d,%s"),
			S.PortalId,
			*S.PortalDefId.ToString(),
			S.Location.X,
			S.Location.Y,
			S.Location.Z,
			S.SettlementValueAtSpawn,
			S.DungeonDifficulty,
			S.SpawnTime,
			S.RaidTimer,
			S.DungeonSeed,
			static_cast<int32>(S.Status),
			S.bDungeonEntered ? 1 : 0,
			S.bCleared ? 1 : 0,
			*EncodeDungeonAttributes(S.DungeonAttributes)));
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

		if (Fields.Num() < 14 || (Fields[0] != TEXT("V2") && Fields[0] != TEXT("V3")))
		{
			// Pre-difficulty portal snapshots are deliberately not migrated: spawn
			// fresh portals so every active portal has a value-based difficulty.
			continue;
		}

		FPortalState S;
		S.PortalId = FCString::Atoi(*Fields[1]);
		S.PortalDefId = FName(*Fields[2]);
		S.Location.X = FCString::Atof(*Fields[3]);
		S.Location.Y = FCString::Atof(*Fields[4]);
		S.Location.Z = FCString::Atof(*Fields[5]);
		S.SettlementValueAtSpawn = FCString::Atof(*Fields[6]);
		S.DungeonDifficulty = FMath::Max(0.5f, FCString::Atof(*Fields[7]));
		S.SpawnTime = FCString::Atof(*Fields[8]);
		S.RaidTimer = FCString::Atof(*Fields[9]);
		S.DungeonSeed = FCString::Atoi(*Fields[10]);
		S.Status = static_cast<EPortalStatus>(FCString::Atoi(*Fields[11]));
		S.bDungeonEntered = FCString::Atoi(*Fields[12]) != 0;
		S.bCleared = FCString::Atoi(*Fields[13]) != 0;
		if (Fields[0] == TEXT("V3") && Fields.Num() >= 15)
		{
			DecodeDungeonAttributes(Fields[14], S.DungeonAttributes);
		}

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

FString UWS_PortalDirector::EncodeSpawnTimers(const TMap<FName, float>& InTimers)
{
	TArray<FString> Items;
	Items.Reserve(InTimers.Num());

	for (const TPair<FName, float>& Pair : InTimers)
	{
		Items.Add(FString::Printf(TEXT("%s=%.3f"), *Pair.Key.ToString(), Pair.Value));
	}

	return FString::Join(Items, TEXT(";"));
}

void UWS_PortalDirector::DecodeSpawnTimers(const FString& Encoded, TMap<FName, float>& OutTimers)
{
	OutTimers.Reset();

	if (Encoded.IsEmpty())
	{
		return;
	}

	TArray<FString> Entries;
	Encoded.ParseIntoArray(Entries, TEXT(";"), true);

	for (const FString& Entry : Entries)
	{
		FString KeyString;
		FString ValueString;
		if (!Entry.Split(TEXT("="), &KeyString, &ValueString))
		{
			continue;
		}

		OutTimers.Add(FName(*KeyString), FCString::Atof(*ValueString));
	}
}

void UWS_PortalDirector::WriteToSnapshot_Implementation(FEidosWorldSnapshot& OutSnapshot) const
{
	OutSnapshot.SetKVString(Key_NextPortalId(), FString::FromInt(NextPortalId));
	OutSnapshot.SetKVString(Key_TimeSinceLastSpawn(), FString::SanitizeFloat(TimeSinceLastSpawn));
	OutSnapshot.SetKVString(Key_SpawnTimers(), EncodeSpawnTimers(TimeSinceLastSpawnByDef));
	OutSnapshot.SetKVString(Key_ActivePortals(), EncodePortalStates(ActivePortals));
}

void UWS_PortalDirector::ApplySnapshot_Implementation(const FEidosWorldSnapshot& Snapshot)
{
	ActivePortals.Reset();
	PortalActors.Reset();
	TimeSinceLastSpawnByDef.Reset();

	NextPortalId = FCString::Atoi(*Snapshot.GetKVString(Key_NextPortalId(), TEXT("1")));
	TimeSinceLastSpawn = FCString::Atof(*Snapshot.GetKVString(Key_TimeSinceLastSpawn(), TEXT("0")));
	DecodeSpawnTimers(Snapshot.GetKVString(Key_SpawnTimers(), TEXT("")), TimeSinceLastSpawnByDef);
	if (TimeSinceLastSpawn > 0.f)
	{
		TimeSinceLastSpawnByDef.FindOrAdd(TEXT("DefaultPortal")) = TimeSinceLastSpawn;
	}

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
