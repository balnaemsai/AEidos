// Fill out your copyright notice in the Description page of Project Settings.


#include "World/Settlement/WS_RaidDirector.h"

#include "Core/Types/PortalTypes.h"
#include "Entities/Page/PageCharacter.h"
#include "Entities/Page/Components/StatsComponent.h"
#include "Engine/World.h"
#include "AIController.h"
#include "Combat/WS_CombatDirector.h"
#include "EngineUtils.h"
#include "Framework/EidosGameMode.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "World/Settlement/WS_ItemStorage.h"
#include "World/Settlement/WS_Population.h"
#include "World/Settlement/WS_PortalDirector.h"
#include "World/Settlement/SettlementCoreActor.h"

void UWS_RaidDirector::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ActiveRaids.Reset();
	PlannedResolvedRaids.Reset();
	PlannedDirectMoves.Reset();
	PlannedCoreDamage = 0.f;
	bSettlementDefeated = false;
	bWarnedMissingSettlementCore = false;
}

void UWS_RaidDirector::StartRaid(const FPortalState& Portal)
{
	if (Portal.PortalId == INDEX_NONE || ActiveRaids.Contains(Portal.PortalId))
	{
		return;
	}

	FRaidRuntime& Raid = ActiveRaids.Add(Portal.PortalId);
	Raid.PortalId = Portal.PortalId;
	Raid.DungeonDifficulty = FMath::Max(0.5f, Portal.DungeonDifficulty);
	Raid.SettlementValueAtSpawn = Portal.SettlementValueAtSpawn;
	Raid.DungeonAttributes = Portal.DungeonAttributes;
	Raid.TotalWaves = FMath::Clamp(FMath::CeilToInt(Raid.DungeonDifficulty), 1, 5);
	Raid.PortalLocation = Portal.Location;
	SpawnNextWave(Raid);
}

bool UWS_RaidDirector::IsRaidActive(int32 PortalId) const
{
	return ActiveRaids.Contains(PortalId);
}

bool UWS_RaidDirector::GetRaidWaveProgress(int32 PortalId, int32& OutCurrentWave, int32& OutTotalWaves) const
{
	OutCurrentWave = 0;
	OutTotalWaves = 0;
	if (const FRaidRuntime* Raid = ActiveRaids.Find(PortalId))
	{
		OutCurrentWave = Raid->CurrentWave;
		OutTotalWaves = Raid->TotalWaves;
		return true;
	}
	return false;
}

UClass* UWS_RaidDirector::ResolveRaiderClass() const
{
	if (const UWS_Population* Population = GetWorld() ? GetWorld()->GetSubsystem<UWS_Population>() : nullptr)
	{
		for (const TWeakObjectPtr<APageCharacter>& Page : Population->GetOwnedPages())
		{
			if (Page.IsValid())
			{
				return Page->GetClass();
			}
		}
	}

	return APageCharacter::StaticClass();
}

void UWS_RaidDirector::SpawnNextWave(FRaidRuntime& Raid)
{
	UWorld* World = GetWorld();
	UClass* RaiderClass = ResolveRaiderClass();
	if (!World || !RaiderClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Raid] Wave spawn failed PortalId=%d: no world or raider class"), Raid.PortalId);
		return;
	}

	++Raid.CurrentWave;
	Raid.Raiders.Reset();
	const int32 RaiderCount = FMath::Clamp(FMath::CeilToInt(Raid.DungeonDifficulty) + Raid.CurrentWave, 2, 10);
	for (int32 Index = 0; Index < RaiderCount; ++Index)
	{
		const float Angle = (2.f * PI * Index) / RaiderCount;
		const FVector Offset(FMath::Cos(Angle) * 180.f, FMath::Sin(Angle) * 180.f, 80.f);

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		if (APageCharacter* Raider = World->SpawnActor<APageCharacter>(RaiderClass, Raid.PortalLocation + Offset, FRotator::ZeroRotator, Params))
		{
			Raider->SetFaction(EPageFaction::Hostile);
			Raider->SetIsInDungeon(false);
			Raider->CurrentJobState = FPageJobState{};
			if (UStatsComponent* Stats = Raider->GetStats())
			{
				Stats->ApplyDifficultyScale(Raid.DungeonDifficulty);
			}
			if (UCharacterMovementComponent* Movement = Raider->GetCharacterMovement())
			{
				Movement->MaxWalkSpeed = RaiderMoveSpeedCmPerSecond;
			}
			Raid.Raiders.Add(Raider);
		}
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[Raid] Wave started PortalId=%d Difficulty=%.2f Wave=%d/%d Raiders=%d"),
		Raid.PortalId,
		Raid.DungeonDifficulty,
		Raid.CurrentWave,
		Raid.TotalWaves,
		Raid.Raiders.Num());
}

ASettlementCoreActor* UWS_RaidDirector::FindSettlementCore() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<ASettlementCoreActor> It(World); It; ++It)
	{
		if (ASettlementCoreActor* Core = *It; IsValid(Core) && !Core->IsDestroyed())
		{
			Core->OnCoreDestroyed.AddUniqueDynamic(this, &UWS_RaidDirector::HandleSettlementCoreDestroyed);
			return Core;
		}
	}
	return nullptr;
}

void UWS_RaidDirector::MoveRaidersTowardCore(FRaidRuntime& Raid, ASettlementCoreActor* SettlementCore, float FixedDeltaSeconds)
{
	if (!SettlementCore)
	{
		return;
	}

	Raid.RepathCooldownSeconds = FMath::Max(0.f, Raid.RepathCooldownSeconds - FixedDeltaSeconds);
	const bool bRequestPathNow = Raid.RepathCooldownSeconds <= 0.f;
	const UWS_CombatDirector* CombatDirector = GetWorld() ? GetWorld()->GetSubsystem<UWS_CombatDirector>() : nullptr;
	for (const TWeakObjectPtr<APageCharacter>& WeakRaider : Raid.Raiders)
	{
		APageCharacter* Raider = WeakRaider.Get();
		if (!Raider || Raider->IsCaptive() || (Raider->GetStats() && (Raider->GetStats()->IsDead() || Raider->GetStats()->IsDowned())))
		{
			continue;
		}
		if (CombatDirector && CombatDirector->IsCombatant(Raider))
		{
			// Stop a previously-issued MoveTo as soon as this raider joins combat.
			if (AAIController* Controller = Cast<AAIController>(Raider->GetController()))
			{
				Controller->StopMovement();
			}
			continue;
		}
		const float DistanceSquared = FVector::DistSquared(Raider->GetActorLocation(), SettlementCore->GetActorLocation());
		if (DistanceSquared <= FMath::Square(CoreAttackRangeCm))
		{
			continue;
		}

		// A MoveTo request can succeed while streamed-level navigation is not yet
		// traversable. Prepare a low-speed fallback and apply it only if stationary.
		FRaidDirectMove& DirectMove = PlannedDirectMoves.AddDefaulted_GetRef();
		DirectMove.Raider = Raider;
		DirectMove.Destination = SettlementCore->GetActorLocation();

		if (bRequestPathNow)
		{
			AAIController* Controller = Cast<AAIController>(Raider->GetController());
			if (!Controller)
			{
				FActorSpawnParameters ControllerParams;
				ControllerParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				Controller = GetWorld()->SpawnActor<AAIController>(AAIController::StaticClass(), Raider->GetActorLocation(), Raider->GetActorRotation(), ControllerParams);
				if (Controller)
				{
					Controller->Possess(Raider);
				}
			}

			if (Controller)
			{
				Controller->MoveToActor(SettlementCore, CoreAttackRangeCm, true, true, true, nullptr, true);
			}
		}
	}

	if (bRequestPathNow)
	{
		Raid.RepathCooldownSeconds = RepathIntervalSeconds;
	}
}

void UWS_RaidDirector::ResolveSettlementDefeat()
{
	if (bSettlementDefeated)
	{
		return;
	}
	bSettlementDefeated = true;
	TArray<int32> FailedPortalIds;
	ActiveRaids.GetKeys(FailedPortalIds);
	ActiveRaids.Reset();
	PlannedResolvedRaids.Reset();
	PlannedDirectMoves.Reset();
	if (UWS_PortalDirector* PortalDirector = GetWorld() ? GetWorld()->GetSubsystem<UWS_PortalDirector>() : nullptr)
	{
		for (const int32 PortalId : FailedPortalIds)
		{
			PortalDirector->FailRaid(PortalId);
		}
	}
	UE_LOG(LogTemp, Error, TEXT("[Raid] Settlement core destroyed. Settlement defeated."));
	if (AEidosGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEidosGameMode>() : nullptr)
	{
		GameMode->TriggerGameOver();
	}
	OnSettlementDefeated.Broadcast();
}

void UWS_RaidDirector::HandleSettlementCoreDestroyed(ASettlementCoreActor* DestroyedCore)
{
	ResolveSettlementDefeat();
}

bool UWS_RaidDirector::HasActiveRaider(const FRaidRuntime& Raid) const
{
	for (const TWeakObjectPtr<APageCharacter>& Raider : Raid.Raiders)
	{
		if (!Raider.IsValid() || Raider->IsCaptive())
		{
			continue;
		}

		if (const UStatsComponent* Stats = Raider->GetStats(); !Stats || (!Stats->IsDead() && !Stats->IsDowned()))
		{
			return true;
		}
	}
	return false;
}

void UWS_RaidDirector::SimPlan_Implementation(USimCommandBuffer* CommandBuffer, float FixedDeltaSeconds)
{
	PlannedResolvedRaids.Reset();
	// Plans are single fixed-tick commands. Keeping these entries across ticks made
	// one stalled raider receive multiple direct-move steps and accelerate over time.
	PlannedDirectMoves.Reset();
	PlannedCoreDamage = 0.f;
	if (bSettlementDefeated)
	{
		return;
	}
	ASettlementCoreActor* SettlementCore = FindSettlementCore();
	const UWS_CombatDirector* CombatDirector = GetWorld() ? GetWorld()->GetSubsystem<UWS_CombatDirector>() : nullptr;
	if (!SettlementCore && !ActiveRaids.IsEmpty() && !bWarnedMissingSettlementCore)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Raid] No SettlementCoreActor exists. Raiders cannot attack until one is placed."));
		bWarnedMissingSettlementCore = true;
	}
	for (TPair<int32, FRaidRuntime>& Pair : ActiveRaids)
	{
		if (SettlementCore)
		{
			MoveRaidersTowardCore(Pair.Value, SettlementCore, FixedDeltaSeconds);
			for (const TWeakObjectPtr<APageCharacter>& WeakRaider : Pair.Value.Raiders)
			{
				if (const APageCharacter* Raider = WeakRaider.Get(); Raider && (!Raider->GetStats() || (!Raider->GetStats()->IsDead() && !Raider->GetStats()->IsDowned())) && !Raider->IsCaptive()
					&& !(CombatDirector && CombatDirector->IsCombatant(Raider))
					&& FVector::DistSquared(Raider->GetActorLocation(), SettlementCore->GetActorLocation()) <= FMath::Square(CoreAttackRangeCm))
				{
					PlannedCoreDamage += RaiderCoreDamagePerSecond * FMath::Max(0.5f, Pair.Value.DungeonDifficulty) * FixedDeltaSeconds;
				}
			}
		}
		if (!HasActiveRaider(Pair.Value))
		{
			PlannedResolvedRaids.Add(Pair.Key);
		}
	}
}

void UWS_RaidDirector::SimCommit_Implementation(USimCommandBuffer* CommandBuffer, float FixedDeltaSeconds)
{
	for (const FRaidDirectMove& DirectMove : PlannedDirectMoves)
	{
		APageCharacter* Raider = DirectMove.Raider.Get();
		if (!Raider || Raider->IsCaptive() || (Raider->GetStats() && (Raider->GetStats()->IsDead() || Raider->GetStats()->IsDowned())))
		{
			continue;
		}

		if (Raider->GetVelocity().SizeSquared2D() > FMath::Square(5.f))
		{
			continue;
		}

		const FVector Delta = DirectMove.Destination - Raider->GetActorLocation();
		const FVector Direction = Delta.GetSafeNormal2D();
		if (!Direction.IsNearlyZero())
		{
			Raider->SetActorLocation(
				Raider->GetActorLocation() + Direction * RaiderMoveSpeedCmPerSecond * FixedDeltaSeconds,
				true);
		}
	}

	if (ASettlementCoreActor* SettlementCore = FindSettlementCore(); SettlementCore && PlannedCoreDamage > 0.f)
	{
		SettlementCore->ApplyCoreDamage(PlannedCoreDamage);
		if (SettlementCore->IsDestroyed())
		{
			ResolveSettlementDefeat();
			return;
		}
	}
	for (const int32 PortalId : PlannedResolvedRaids)
	{
		FRaidRuntime* Raid = ActiveRaids.Find(PortalId);
		if (!Raid)
		{
			continue;
		}

		if (Raid->CurrentWave < Raid->TotalWaves)
		{
			SpawnNextWave(*Raid);
		}
		else
		{
			CompleteRaid(PortalId);
		}
	}
}

void UWS_RaidDirector::CompleteRaid(int32 PortalId)
{
	FRaidRuntime* Raid = ActiveRaids.Find(PortalId);
	if (!Raid)
	{
		return;
	}

	const int32 RewardQuantity = FMath::Max(0, FMath::RoundToInt(Raid->DungeonDifficulty * RaidRewardPerDifficulty));
	if (!RaidRewardItemId.IsNone() && RewardQuantity > 0)
	{
		UWS_ItemStorage* Storage = GetWorld() ? GetWorld()->GetSubsystem<UWS_ItemStorage>() : nullptr;
		FItemStack PortalShard;
		PortalShard.ItemId = RaidRewardItemId;
		PortalShard.Quantity = 1;
		PortalShard.DungeonAttributes = Raid->DungeonAttributes;
		const TArray<FItemStack> RewardStacks = { PortalShard };
		if (!Storage || !Storage->CanStoreItemStacks(RewardStacks))
		{
			if (!Raid->bAwaitingRewardStorage)
			{
				Raid->bAwaitingRewardStorage = true;
				UE_LOG(LogTemp, Warning,
					TEXT("[Raid] Defense complete PortalId=%d but warehouse has no space for %s x%d. Keep the portal open until space is available."),
					PortalId,
					*RaidRewardItemId.ToString(),
					RewardQuantity);
			}
			return;
		}

		const int32 StoredQuantity = Storage->TryStoreItemStack(PortalShard);
		if (StoredQuantity != PortalShard.Quantity)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[Raid] Defense reward storage changed unexpectedly PortalId=%d Expected=%d Stored=%d"),
				PortalId,
				RewardQuantity,
				StoredQuantity);
			return;
		}
	}

	FRaidRuntime CompletedRaid;
	if (!ActiveRaids.RemoveAndCopyValue(PortalId, CompletedRaid))
	{
		return;
	}

	if (UWS_PortalDirector* PortalDirector = GetWorld() ? GetWorld()->GetSubsystem<UWS_PortalDirector>() : nullptr)
	{
		PortalDirector->ResolveRaid(PortalId);
	}

	UE_LOG(LogTemp, Log, TEXT("[Raid] Resolved PortalId=%d Difficulty=%.2f Reward=%s x%d"),
		PortalId,
		CompletedRaid.DungeonDifficulty,
		*RaidRewardItemId.ToString(),
		RewardQuantity);
}

void UWS_RaidDirector::SimPost_Implementation(float FixedDeltaSeconds)
{
}

