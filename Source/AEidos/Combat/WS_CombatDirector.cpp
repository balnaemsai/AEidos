// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/WS_CombatDirector.h"
#include "Entities/Page/Components/SkillComponent.h"

#include "Data/Definitions/SkillDefinitionRow.h"
#include "World/Dungeon/DungeonCoreActor.h"
#include "Data/GIS_DataRegistry.h"
#include "Engine/GameInstance.h"
#include "Entities/Page/PageCharacter.h"
#include "Entities/Page/Components/StatsComponent.h"
#include "World/Settlement/WS_Population.h"
#include "Framework/EidosPlayerController.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Player/Camera/CameraModeComponent.h"
#include "Simulation/SimCommandBuffer.h"

TArray<APageCharacter*> UWS_CombatDirector::GatherLivingPages() const
{
	TArray<APageCharacter*> Pages;
	if (!GetWorld())
	{
		return Pages;
	}

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APageCharacter::StaticClass(), FoundActors);

	for (AActor* Actor : FoundActors)
	{
		APageCharacter* Page = Cast<APageCharacter>(Actor);
		if (!Page)
		{
			continue;
		}

		const UStatsComponent* Stats = Page->GetStats();
		if (!Stats || Stats->IsDead() || Page->IsCaptive())
		{
			continue;
		}

		Pages.Add(Page);
	}

	return Pages;
}

ADungeonCoreActor* UWS_CombatDirector::FindLivingDungeonCore() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<ADungeonCoreActor> It(World); It; ++It)
	{
		if (ADungeonCoreActor* Core = *It; IsValid(Core) && Core->GetHealth() > 0.f)
		{
			return Core;
		}
	}

	return nullptr;
}

bool UWS_CombatDirector::HasDungeonCoreObjective() const
{
	if (!ActiveEncounter.bCombatSpaceIsDungeon)
	{
		return false;
	}

	const ADungeonCoreActor* Core = FindLivingDungeonCore();
	if (!Core)
	{
		return false;
	}

	// A distant core must not keep a cleared local encounter in combat.
	for (const TWeakObjectPtr<APageCharacter>& WeakCombatant : ActiveEncounter.Combatants)
	{
		if (const APageCharacter* Combatant = WeakCombatant.Get())
		{
			if (Combatant->IsFriendly()
				&& FVector::DistSquared(Combatant->GetActorLocation(), Core->GetActorLocation()) <= FMath::Square(CombatJoinRangeCm))
			{
				return true;
			}
		}
	}

	return false;
}

APageCharacter* UWS_CombatDirector::FindClosestHostileTarget(APageCharacter* Source, const TArray<APageCharacter*>& Candidates, float MaxRangeCm) const
{
	if (!Source)
	{
		return nullptr;
	}

	APageCharacter* ClosestEnemy = nullptr;
	float ClosestDistSq = FMath::Square(MaxRangeCm);

	for (APageCharacter* Candidate : Candidates)
	{
		if (!Candidate || !Source->IsHostileTo(Candidate))
		{
			continue;
		}

		if (Source->IsInDungeon() != Candidate->IsInDungeon())
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(Source->GetActorLocation(), Candidate->GetActorLocation());
		if (DistSq < ClosestDistSq)
		{
			ClosestDistSq = DistSq;
			ClosestEnemy = Candidate;
		}
	}

	return ClosestEnemy;
}

TArray<APageCharacter*> UWS_CombatDirector::GatherEncounterSeedCombatants(
	APageCharacter* TriggerPage,
	APageCharacter* TriggerTarget,
	const TArray<APageCharacter*>& Candidates) const
{
	TArray<APageCharacter*> SeedCombatants;
	if (!TriggerPage || !TriggerTarget)
	{
		return SeedCombatants;
	}

	const bool bDungeonSpace = TriggerPage->IsInDungeon();
	TArray<APageCharacter*> Frontier;
	Frontier.AddUnique(TriggerPage);
	Frontier.AddUnique(TriggerTarget);
	SeedCombatants = Frontier;

	while (Frontier.Num() > 0)
	{
		APageCharacter* Anchor = Frontier.Pop(EAllowShrinking::No);
		if (!Anchor)
		{
			continue;
		}

		for (APageCharacter* Candidate : Candidates)
		{
			if (!Candidate || Candidate->IsInDungeon() != bDungeonSpace || SeedCombatants.Contains(Candidate))
			{
				continue;
			}

			const float DistSq = FVector::DistSquared(Anchor->GetActorLocation(), Candidate->GetActorLocation());
			if (DistSq <= FMath::Square(CombatJoinRangeCm))
			{
				SeedCombatants.Add(Candidate);
				Frontier.Add(Candidate);
			}
		}
	}

	return SeedCombatants;
}

bool UWS_CombatDirector::TryStartEncounter(const TArray<APageCharacter*>& Pages)
{
	for (APageCharacter* Page : Pages)
	{
		if (!Page)
		{
			continue;
		}

		if (APageCharacter* Target = FindClosestHostileTarget(Page, Pages, EncounterStartRangeCm))
		{
			const TArray<APageCharacter*> Combatants = GatherEncounterSeedCombatants(Page, Target, Pages);

			StartEncounter(Combatants, Page->IsInDungeon());
			UE_LOG(LogTemp, Log,
				TEXT("[Combat] Encounter started by %s vs %s (Combatants=%d, Dungeon=%d)"),
				*GetNameSafe(Page),
				*GetNameSafe(Target),
				Combatants.Num(),
				Page->IsInDungeon() ? 1 : 0);
			return true;
		}
	}

	// The dungeon core is a hostile objective rather than a turn-taking Page.
	// It keeps a friendly-only dungeon encounter alive until the core is destroyed.
	if (ADungeonCoreActor* Core = FindLivingDungeonCore())
	{
		for (APageCharacter* Page : Pages)
		{
			if (!Page || !Page->IsFriendly() || !Page->IsInDungeon())
			{
				continue;
			}

			if (FVector::DistSquared(Page->GetActorLocation(), Core->GetActorLocation()) <= FMath::Square(EncounterStartRangeCm))
			{
				StartEncounter({Page}, true);
				UE_LOG(LogTemp, Log, TEXT("[Combat] Core objective encounter started by %s"), *GetNameSafe(Page));
				return true;
			}
		}
	}

	return false;
}

void UWS_CombatDirector::StartEncounter(const TArray<APageCharacter*>& Combatants, bool bCombatSpaceIsDungeon)
{
	EndEncounter(TEXT("RestartEncounter"));

	ActiveEncounter = FCombatEncounterRuntime{};
	ActiveEncounter.EncounterId = NextEncounterId++;
	ActiveEncounter.RoundIndex = 1;
	ActiveEncounter.ActiveTurnIndex = 0;
	ActiveEncounter.TurnTimeRemaining = TurnTimeLimitSeconds;
	ActiveEncounter.bCombatSpaceIsDungeon = bCombatSpaceIsDungeon;

	bAdvanceTurnRequested = false;
	PendingFriendlyAction = FPendingCombatActionRequest{};

	for (APageCharacter* Combatant : Combatants)
	{
		if (!Combatant)
		{
			continue;
		}

		ActiveEncounter.Combatants.Add(Combatant);
		FCombatActionPointState& APState = CombatantActionPoints.FindOrAdd(Combatant);
		APState.MaxActionPoints = GetActionPointsPerTurn(Combatant);
		APState.ActionPointsRemaining = 0;
		APState.MovementProgressCm = 0.f;

		FCombatInitiativeState& InitiativeState = CombatantInitiative.FindOrAdd(Combatant);
		InitiativeState.InitiativeValue = 0.f;
		InitiativeState.AgilityValue = GetCombatAgility(Combatant);
		InitiativeState.ActionThreshold = GetActionThreshold(Combatant);
		InitiativeState.TurnsTaken = 0;
	}

	CombatantActionPoints.Compact();
	CombatantInitiative.Compact();
	AdvanceTurn();
	FocusActiveFriendlyPage();
}

void UWS_CombatDirector::RebuildCombatantsFromWorld(const TArray<APageCharacter*>& Pages)
{
	if (!IsCombatActive())
	{
		return;
	}

	TArray<TWeakObjectPtr<APageCharacter>> RebuiltCombatants;
	for (const TWeakObjectPtr<APageCharacter>& ExistingCombatant : ActiveEncounter.Combatants)
	{
		APageCharacter* ExistingPage = ExistingCombatant.Get();
		if (!ExistingPage)
		{
			continue;
		}

		if (Pages.Contains(ExistingPage))
		{
			RebuiltCombatants.Add(ExistingPage);
		}
	}

	ActiveEncounter.Combatants = MoveTemp(RebuiltCombatants);
	CombatantActionPoints.Compact();
	CombatantInitiative.Compact();
	if (ActiveEncounter.Combatants.Num() > 0)
	{
		ActiveEncounter.ActiveTurnIndex = ActiveEncounter.ActiveTurnIndex % ActiveEncounter.Combatants.Num();
	}
}

void UWS_CombatDirector::RecruitNearbyCombatants(const TArray<APageCharacter*>& Pages)
{
	if (!IsCombatActive())
	{
		return;
	}

	for (APageCharacter* Candidate : Pages)
	{
		if (!Candidate || Candidate->IsInDungeon() != ActiveEncounter.bCombatSpaceIsDungeon)
		{
			continue;
		}

		if (ActiveEncounter.Combatants.Contains(Candidate))
		{
			continue;
		}

		bool bShouldJoin = false;
		for (const TWeakObjectPtr<APageCharacter>& WeakCombatant : ActiveEncounter.Combatants)
		{
			if (const APageCharacter* Combatant = WeakCombatant.Get())
			{
				const float DistSq = FVector::DistSquared(Combatant->GetActorLocation(), Candidate->GetActorLocation());
				if (DistSq <= FMath::Square(CombatJoinRangeCm))
				{
					bShouldJoin = true;
					break;
				}
			}
		}

		if (!bShouldJoin)
		{
			continue;
		}

		ActiveEncounter.Combatants.Add(Candidate);

		FCombatActionPointState& APState = CombatantActionPoints.FindOrAdd(Candidate);
		APState.MaxActionPoints = GetActionPointsPerTurn(Candidate);
		APState.ActionPointsRemaining = 0;
		APState.MovementProgressCm = 0.f;

		FCombatInitiativeState& InitiativeState = CombatantInitiative.FindOrAdd(Candidate);
		InitiativeState.InitiativeValue = 0.f;
		InitiativeState.AgilityValue = GetCombatAgility(Candidate);
		InitiativeState.ActionThreshold = GetActionThreshold(Candidate);
		InitiativeState.TurnsTaken = 0;

		Candidate->SetTurnCombatState(true, false);
		UE_LOG(LogTemp, Log,
			TEXT("[Combat] %s joined encounter %d by proximity"),
			*GetNameSafe(Candidate),
			ActiveEncounter.EncounterId);
	}
}

void UWS_CombatDirector::RefreshEncounterState()
{
	if (!IsCombatActive())
	{
		return;
	}

	int32 FriendlyCount = 0;
	int32 HostileCount = 0;
	for (const TWeakObjectPtr<APageCharacter>& WeakCombatant : ActiveEncounter.Combatants)
	{
		if (const APageCharacter* Combatant = WeakCombatant.Get())
		{
			if (Combatant->IsFriendly())
			{
				++FriendlyCount;
			}
			else if (Combatant->IsHostile())
			{
				++HostileCount;
			}
		}
	}

	// A living dungeon core is a hostile objective even though it does not take turns.
	if (FriendlyCount == 0 || (HostileCount == 0 && !HasDungeonCoreObjective()))
	{
		EndEncounter(TEXT("OneFactionRemaining"));
		return;
	}

	if (APageCharacter* ActivePage = GetActiveCombatant())
	{
		ActiveEncounter.State = ActivePage->IsFriendly()
			? ECombatEncounterState::PlayerTurn
			: ECombatEncounterState::EnemyTurn;
	}
	else
	{
		EndEncounter(TEXT("NoActiveCombatant"));
	}
}

void UWS_CombatDirector::NotifyCombatantDefeated(APageCharacter* DefeatedPage)
{
	if (!IsCombatActive() || !DefeatedPage)
	{
		return;
	}

	ActiveEncounter.Combatants.RemoveAll([DefeatedPage](const TWeakObjectPtr<APageCharacter>& Combatant)
	{
		return !Combatant.IsValid() || Combatant.Get() == DefeatedPage;
	});
	CombatantActionPoints.Remove(DefeatedPage);
	CombatantInitiative.Remove(DefeatedPage);

	if (ActiveEncounter.Combatants.Num() == 0)
	{
		EndEncounter(TEXT("NoLivingCombatants"));
		return;
	}

	ActiveEncounter.ActiveTurnIndex %= ActiveEncounter.Combatants.Num();
	RefreshEncounterState();
}

void UWS_CombatDirector::UpdateCombatantTurnFlags(APageCharacter* ActivePage)
{
	for (const TWeakObjectPtr<APageCharacter>& WeakCombatant : ActiveEncounter.Combatants)
	{
		if (APageCharacter* Combatant = WeakCombatant.Get())
		{
			Combatant->SetTurnCombatState(true, Combatant == ActivePage);
		}
	}
}

void UWS_CombatDirector::BeginTurnForCombatant(APageCharacter* ActivePage)
{
	if (!ActivePage)
	{
		return;
	}

	FCombatActionPointState& APState = CombatantActionPoints.FindOrAdd(ActivePage);
	APState.MaxActionPoints = GetActionPointsPerTurn(ActivePage);
	APState.ActionPointsRemaining = APState.MaxActionPoints;
	APState.MovementProgressCm = 0.f;

	bAdvanceTurnRequested = false;
	PendingFriendlyAction = FPendingCombatActionRequest{};
	UpdateCombatantTurnFlags(ActivePage);
	ActiveEncounter.TurnTimeRemaining = TurnTimeLimitSeconds;
}

bool UWS_CombatDirector::HasActionPointsRemaining(APageCharacter* Page) const
{
	if (!Page)
	{
		return false;
	}

	if (const FCombatActionPointState* APState = CombatantActionPoints.Find(Page))
	{
		return APState->ActionPointsRemaining > 0;
	}

	return false;
}

float UWS_CombatDirector::GetCombatAgility(const APageCharacter* Page) const
{
	const UStatsComponent* Stats = Page ? Page->GetStats() : nullptr;
	return Stats ? Stats->GetCombatAgility() : 5.f;
}

float UWS_CombatDirector::GetActionThreshold(const APageCharacter* Page) const
{
	const UStatsComponent* Stats = Page ? Page->GetStats() : nullptr;
	return Stats ? Stats->GetCombatActionThreshold() : 10.f;
}

int32 UWS_CombatDirector::GetActionPointsPerTurn(const APageCharacter* Page) const
{
	const UStatsComponent* Stats = Page ? Page->GetStats() : nullptr;
	return Stats ? Stats->GetCombatActionPointsPerTurn() : 2;
}

bool UWS_CombatDirector::TrySpendActionPoints(APageCharacter* Page, int32 Cost, const TCHAR* Context)
{
	if (!Page || Cost <= 0)
	{
		return false;
	}

	FCombatActionPointState* APState = CombatantActionPoints.Find(Page);
	if (!APState || APState->ActionPointsRemaining < Cost)
	{
		if (APState && APState->ActionPointsRemaining <= 0)
		{
			bAdvanceTurnRequested = true;
		}
		return false;
	}

	APState->ActionPointsRemaining -= Cost;
	UE_LOG(LogTemp, Log,
		TEXT("[Combat] %s spent %d AP for %s (Remaining=%d)"),
		*GetNameSafe(Page),
		Cost,
		Context ? Context : TEXT("Action"),
		APState->ActionPointsRemaining);

	if (APState->ActionPointsRemaining <= 0)
	{
		bAdvanceTurnRequested = true;
	}

	return true;
}

const FSkillDefinitionRow* UWS_CombatDirector::GetActiveSkillDefinition(FName SkillId) const
{
	if (SkillId.IsNone() || !GetWorld())
	{
		return nullptr;
	}

	UGameInstance* GI = GetWorld()->GetGameInstance();
	UGIS_DataRegistry* Registry = GI ? GI->GetSubsystem<UGIS_DataRegistry>() : nullptr;
	if (!Registry || !Registry->EnsureReadySync())
	{
		return nullptr;
	}

	return Registry->GetSkillDef(SkillId);
}

bool UWS_CombatDirector::ExecutePendingFriendlyAction(USimCommandBuffer* Cmd, APageCharacter* ActivePage, const TArray<APageCharacter*>& Pages)
{
	if (!Cmd || !ActivePage || !ActivePage->IsFriendly())
	{
		return false;
	}

	if (PendingFriendlyAction.RequestingPage.Get() != ActivePage)
	{
		return false;
	}

	FPendingCombatActionRequest Request = PendingFriendlyAction;
	PendingFriendlyAction = FPendingCombatActionRequest{};

	if (Request.ActionType == EPageCombatActionType::EndTurn)
	{
		bAdvanceTurnRequested = true;
		return true;
	}

	if (Request.ActionType == EPageCombatActionType::ItemUse)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Combat] Item action is not implemented yet: %s"), *Request.ActionId.ToString());
		return false;
	}

	if (Request.ActionType != EPageCombatActionType::ActiveSkill)
	{
		return false;
	}

	const FSkillDefinitionRow* SkillDef = GetActiveSkillDefinition(Request.ActionId);
	if (!SkillDef || !SkillDef->bIsActiveCombatSkill)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Combat] Active skill definition missing or not active: %s"), *Request.ActionId.ToString());
		return false;
	}

	if (!ActivePage->Skills || !ActivePage->Skills->HasSkill(Request.ActionId))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Combat] %s does not own active skill %s"), *GetNameSafe(ActivePage), *Request.ActionId.ToString());
		return false;
	}

	AActor* TargetActor = Request.TargetActor.Get();
	APageCharacter* TargetPage = Cast<APageCharacter>(TargetActor);
	ADungeonCoreActor* TargetCore = Cast<ADungeonCoreActor>(TargetActor);
	if (SkillDef->bRequiresTarget)
	{
		if (!TargetActor)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Combat] Skill %s requires a target"), *Request.ActionId.ToString());
			return false;
		}

		if (!TargetPage && !TargetCore)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Combat] Skill %s has an unsupported target %s"), *Request.ActionId.ToString(), *GetNameSafe(TargetActor));
			return false;
		}

		if (SkillDef->bTargetHostileOnly && TargetPage && !ActivePage->IsHostileTo(TargetPage))
		{
			return false;
		}

		if (TargetPage && ActivePage->IsInDungeon() != TargetPage->IsInDungeon())
		{
			return false;
		}

		if (TargetCore && !ActivePage->IsInDungeon())
		{
			return false;
		}

		const float Distance = FVector::Distance(ActivePage->GetActorLocation(), TargetActor->GetActorLocation());
		if (Distance > SkillDef->CombatRangeCm)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[Combat] Skill %s target out of range (%.1f > %.1f)"),
				*Request.ActionId.ToString(),
				Distance,
				SkillDef->CombatRangeCm);
			return false;
		}
	}

	const int32 APCost = FMath::Max(0, SkillDef->CombatActionPointCost);
	if (APCost > 0 && !TrySpendActionPoints(ActivePage, APCost, *Request.ActionId.ToString()))
	{
		return false;
	}

	const TWeakObjectPtr<APageCharacter> WeakAttacker = ActivePage;
	const TWeakObjectPtr<AActor> WeakTarget = TargetActor;
	const float Damage = SkillDef->CombatDamageAmount * ActivePage->GetSkillMultiplier(Request.ActionId);
	const FName SkillId = Request.ActionId;
	const bool bCaptureOnDefeat = SkillDef->bCapturesTargetOnDefeat;

	Cmd->Enqueue([WeakAttacker, WeakTarget, Damage, SkillId, bCaptureOnDefeat]()
	{
		APageCharacter* Attacker = WeakAttacker.Get();
		if (!Attacker)
		{
			return;
		}

		if (AActor* TargetActorResolved = WeakTarget.Get())
		{
			if (APageCharacter* Target = Cast<APageCharacter>(TargetActorResolved))
			{
				if (UStatsComponent* TargetStats = Target->GetStats())
				{
					TargetStats->ApplyDamage(Damage);
					Attacker->AddActiveSkillXP(SkillId, FMath::Max(5.f, Damage * 0.25f));

					UE_LOG(LogTemp, Log,
						TEXT("[Combat] %s used %s on %s for %.1f damage (HP %.1f/%.1f)"),
						*GetNameSafe(Attacker),
						*SkillId.ToString(),
						*GetNameSafe(Target),
						Damage,
						TargetStats->GetHealth(),
						TargetStats->GetMaxHealth());

					if (TargetStats->IsDead())
					{
						UWorld* AttackerWorld = Attacker->GetWorld();
						UWS_Population* Population = AttackerWorld
							? AttackerWorld->GetSubsystem<UWS_Population>()
							: nullptr;
						const bool bCaptured = bCaptureOnDefeat && Target->IsHostile()
							&& Population && Population->CaptureHostilePage(Target);
						if (bCaptured)
						{
							UE_LOG(LogTemp, Log, TEXT("[Combat] %s subdued %s"), *GetNameSafe(Attacker), *GetNameSafe(Target));
						}
						else
						{
							UE_LOG(LogTemp, Warning, TEXT("[Combat] %s died"), *GetNameSafe(Target));
						}
						if (UWS_CombatDirector* Combat = AttackerWorld ? AttackerWorld->GetSubsystem<UWS_CombatDirector>() : nullptr)
						{
							Combat->NotifyCombatantDefeated(Target);
						}
						if (!bCaptured)
						{
							Target->Destroy();
						}
					}
				}
			}
			else if (ADungeonCoreActor* TargetCoreResolved = Cast<ADungeonCoreActor>(TargetActorResolved))
			{
				TargetCoreResolved->ApplyCoreDamage(Damage, Attacker);
				Attacker->AddActiveSkillXP(SkillId, FMath::Max(5.f, Damage * 0.25f));
				UE_LOG(LogTemp, Log,
					TEXT("[Combat] %s used %s on dungeon core for %.1f damage (HP %.1f/%.1f)"),
					*GetNameSafe(Attacker),
					*SkillId.ToString(),
					Damage,
					TargetCoreResolved->GetHealth(),
					TargetCoreResolved->GetMaxHealth());
			}
		}
	});

	return true;
}

bool UWS_CombatDirector::ExecuteEnemyTurnStep(USimCommandBuffer* Cmd, APageCharacter* ActivePage, const TArray<APageCharacter*>& Pages)
{
	if (!Cmd || !ActivePage || !ActivePage->IsHostile())
	{
		return false;
	}

	APageCharacter* ClosestEnemy = FindClosestHostileTarget(ActivePage, Pages, DetectionRangeCm);
	if (!ClosestEnemy)
	{
		bAdvanceTurnRequested = true;
		return false;
	}

	const float Distance = FVector::Distance(ActivePage->GetActorLocation(), ClosestEnemy->GetActorLocation());
	if (Distance <= AttackRangeCm)
	{
		if (!TrySpendActionPoints(ActivePage, 1, TEXT("EnemyAttack")))
		{
			return false;
		}

		const TWeakObjectPtr<APageCharacter> WeakAttacker = ActivePage;
		const TWeakObjectPtr<APageCharacter> WeakTarget = ClosestEnemy;
		const float Damage = AttackDamagePerHit;

		Cmd->Enqueue([WeakAttacker, WeakTarget, Damage]()
		{
			APageCharacter* Attacker = WeakAttacker.Get();
			APageCharacter* Target = WeakTarget.Get();
			if (!Attacker || !Target)
			{
				return;
			}

			if (UStatsComponent* TargetStats = Target->GetStats())
			{
				TargetStats->ApplyDamage(Damage);

				UE_LOG(LogTemp, Log,
					TEXT("[Combat] %s hit %s for %.1f (HP %.1f/%.1f)"),
					*GetNameSafe(Attacker),
					*GetNameSafe(Target),
					Damage,
					TargetStats->GetHealth(),
					TargetStats->GetMaxHealth());

				if (TargetStats->IsDead())
				{
					UE_LOG(LogTemp, Warning, TEXT("[Combat] %s died"), *GetNameSafe(Target));
					if (UWS_CombatDirector* Combat = Attacker->GetWorld()->GetSubsystem<UWS_CombatDirector>())
					{
						Combat->NotifyCombatantDefeated(Target);
					}
					Target->Destroy();
				}
			}
		});

		return true;
	}

	if (!TrySpendActionPoints(ActivePage, 1, TEXT("EnemyMove")))
	{
		return false;
	}

	const FVector Direction = (ClosestEnemy->GetActorLocation() - ActivePage->GetActorLocation()).GetSafeNormal2D();
	const FVector MoveDelta = Direction * MovementPerTurnCm;
	const TWeakObjectPtr<APageCharacter> WeakPage = ActivePage;

	Cmd->Enqueue([WeakPage, MoveDelta]()
	{
		if (APageCharacter* MovingPage = WeakPage.Get())
		{
			MovingPage->SetActorLocation(MovingPage->GetActorLocation() + MoveDelta, true);
		}
	});

	return true;
}

void UWS_CombatDirector::AdvanceTurn()
{
	if (!IsCombatActive() || ActiveEncounter.Combatants.Num() == 0)
	{
		return;
	}

	APageCharacter* PreviousPage = GetActiveCombatant();
	if (PreviousPage)
	{
		if (FCombatInitiativeState* InitiativeState = CombatantInitiative.Find(PreviousPage))
		{
			InitiativeState->InitiativeValue = 0.f;
			InitiativeState->TurnsTaken += 1;
		}
	}

	int32 SelectedIndex = INDEX_NONE;
	float SelectedInitiative = -1.f;
	float SelectedAgility = -1.f;
	int32 IterationGuard = 0;

	while (SelectedIndex == INDEX_NONE && IterationGuard++ < 128)
	{
		for (int32 Index = 0; Index < ActiveEncounter.Combatants.Num(); ++Index)
		{
			APageCharacter* Combatant = ActiveEncounter.Combatants[Index].Get();
			if (!Combatant)
			{
				continue;
			}

			FCombatInitiativeState& InitiativeState = CombatantInitiative.FindOrAdd(Combatant);
			InitiativeState.AgilityValue = GetCombatAgility(Combatant);
			InitiativeState.ActionThreshold = GetActionThreshold(Combatant);
			InitiativeState.InitiativeValue += InitiativeState.AgilityValue;

			if (InitiativeState.InitiativeValue >= InitiativeState.ActionThreshold)
			{
				if (SelectedIndex == INDEX_NONE
					|| InitiativeState.InitiativeValue > SelectedInitiative
					|| (FMath::IsNearlyEqual(InitiativeState.InitiativeValue, SelectedInitiative) && InitiativeState.AgilityValue > SelectedAgility))
				{
					SelectedIndex = Index;
					SelectedInitiative = InitiativeState.InitiativeValue;
					SelectedAgility = InitiativeState.AgilityValue;
				}
			}
		}
	}

	if (SelectedIndex == INDEX_NONE)
	{
		EndEncounter(TEXT("NoCombatantPassedThreshold"));
		return;
	}

	ActiveEncounter.ActiveTurnIndex = SelectedIndex;
	int32 TotalTurnsTaken = 0;
	for (const TPair<TWeakObjectPtr<APageCharacter>, FCombatInitiativeState>& Pair : CombatantInitiative)
	{
		TotalTurnsTaken += Pair.Value.TurnsTaken;
	}
	ActiveEncounter.RoundIndex = 1 + (ActiveEncounter.Combatants.Num() > 0 ? TotalTurnsTaken / ActiveEncounter.Combatants.Num() : 0);

	RefreshEncounterState();
	BeginTurnForCombatant(GetActiveCombatant());
	FocusActiveFriendlyPage();

	if (APageCharacter* ActivePage = GetActiveCombatant())
	{
		UE_LOG(LogTemp, Log,
			TEXT("[Combat] Encounter=%d Round=%d Turn=%s State=%s"),
			ActiveEncounter.EncounterId,
			ActiveEncounter.RoundIndex,
			*GetNameSafe(ActivePage),
			ActiveEncounter.State == ECombatEncounterState::PlayerTurn ? TEXT("PlayerTurn") : TEXT("EnemyTurn"));
	}
}

void UWS_CombatDirector::EndEncounter(const TCHAR* Reason)
{
	for (const TWeakObjectPtr<APageCharacter>& WeakCombatant : ActiveEncounter.Combatants)
	{
		if (APageCharacter* Combatant = WeakCombatant.Get())
		{
			Combatant->SetTurnCombatState(false, false);
		}
	}

	if (ActiveEncounter.EncounterId != 0)
	{
		UE_LOG(LogTemp, Log,
			TEXT("[Combat] Encounter ended Id=%d Reason=%s"),
			ActiveEncounter.EncounterId,
			Reason ? Reason : TEXT("Unknown"));
	}

	ActiveEncounter = FCombatEncounterRuntime{};
	CombatantActionPoints.Empty();
	CombatantInitiative.Empty();
	bAdvanceTurnRequested = false;
	PendingFriendlyAction = FPendingCombatActionRequest{};
}

void UWS_CombatDirector::FocusActiveFriendlyPage() const
{
	if (!GetWorld())
	{
		return;
	}

	APageCharacter* ActivePage = GetActiveCombatant();
	if (!ActivePage || !ActivePage->IsFriendly())
	{
		return;
	}

	AEidosPlayerController* PC = Cast<AEidosPlayerController>(GetWorld()->GetFirstPlayerController());
	if (!PC || !PC->GetCameraMode())
	{
		return;
	}

	PC->GetCameraMode()->SetSelectedPage(ActivePage);
	PC->GetCameraMode()->FocusSelectedPage(true);
}

APageCharacter* UWS_CombatDirector::GetActiveCombatant() const
{
	if (!IsCombatActive() || ActiveEncounter.Combatants.Num() == 0)
	{
		return nullptr;
	}

	const int32 SafeIndex = FMath::Clamp(ActiveEncounter.ActiveTurnIndex, 0, ActiveEncounter.Combatants.Num() - 1);
	return ActiveEncounter.Combatants[SafeIndex].Get();
}

void UWS_CombatDirector::SimPlan_Implementation(USimCommandBuffer* Cmd, float FixedDeltaSeconds)
{
	if (!Cmd || !GetWorld())
	{
		return;
	}

	const TArray<APageCharacter*> Pages = GatherLivingPages();

	if (!IsCombatActive())
	{
		TryStartEncounter(Pages);
	}
	else
	{
		RebuildCombatantsFromWorld(Pages);
		RecruitNearbyCombatants(Pages);
		RefreshEncounterState();
	}

	if (!IsCombatActive())
	{
		return;
	}

	APageCharacter* ActivePage = GetActiveCombatant();
	if (!ActivePage)
	{
		EndEncounter(TEXT("ActivePageMissing"));
		return;
	}

	UpdateCombatantTurnFlags(ActivePage);

	if (!CombatantActionPoints.Find(ActivePage))
	{
		BeginTurnForCombatant(ActivePage);
	}

	ActiveEncounter.TurnTimeRemaining -= FixedDeltaSeconds;
	if (ActiveEncounter.TurnTimeRemaining <= 0.f)
	{
		bAdvanceTurnRequested = true;
	}

	if (bAdvanceTurnRequested)
	{
		AdvanceTurn();
		return;
	}

	if (ActivePage->IsFriendly())
	{
		ExecutePendingFriendlyAction(Cmd, ActivePage, Pages);
		if (!HasActionPointsRemaining(ActivePage))
		{
			bAdvanceTurnRequested = true;
		}

		if (bAdvanceTurnRequested)
		{
			AdvanceTurn();
		}
		return;
	}

	ExecuteEnemyTurnStep(Cmd, ActivePage, Pages);
	if (!HasActionPointsRemaining(ActivePage))
	{
		bAdvanceTurnRequested = true;
	}

	if (bAdvanceTurnRequested)
	{
		AdvanceTurn();
	}
}

void UWS_CombatDirector::SimCommit_Implementation(USimCommandBuffer* CommandBuffer, float FixedDeltaSeconds)
{
}

void UWS_CombatDirector::SimPost_Implementation(float FixedDeltaSeconds)
{
}

bool UWS_CombatDirector::IsCombatActive() const
{
	return ActiveEncounter.EncounterId != 0 && ActiveEncounter.Combatants.Num() > 0;
}

bool UWS_CombatDirector::IsPageTurnActive(const APageCharacter* Page) const
{
	return Page && GetActiveCombatant() == Page;
}

int32 UWS_CombatDirector::GetActionPointsRemaining(const APageCharacter* Page) const
{
	if (!Page)
	{
		return 0;
	}

	if (const FCombatActionPointState* APState = CombatantActionPoints.Find(Page))
	{
		return APState->ActionPointsRemaining;
	}

	return 0;
}

int32 UWS_CombatDirector::GetMaxActionPoints(const APageCharacter* Page) const
{
	if (!Page)
	{
		return 0;
	}

	if (const FCombatActionPointState* APState = CombatantActionPoints.Find(Page))
	{
		return APState->MaxActionPoints;
	}

	return 0;
}

APageCharacter* UWS_CombatDirector::GetActiveCombatantForUI() const
{
	return GetActiveCombatant();
}

bool UWS_CombatDirector::NotifyPageMoved(APageCharacter* Page, float DistanceCm)
{
	if (!Page || !IsCombatActive() || !Page->IsFriendly() || !IsPageTurnActive(Page) || DistanceCm <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	FCombatActionPointState* APState = CombatantActionPoints.Find(Page);
	if (!APState || APState->ActionPointsRemaining <= 0)
	{
		bAdvanceTurnRequested = true;
		return false;
	}

	APState->MovementProgressCm += DistanceCm;
	while (APState->MovementProgressCm >= MovementPerTurnCm && APState->ActionPointsRemaining > 0)
	{
		APState->MovementProgressCm -= MovementPerTurnCm;
		APState->ActionPointsRemaining -= 1;
		UE_LOG(LogTemp, Log,
			TEXT("[Combat] %s spent 1 AP for movement (Remaining=%d)"),
			*GetNameSafe(Page),
			APState->ActionPointsRemaining);
	}

	if (APState->ActionPointsRemaining <= 0)
	{
		APState->MovementProgressCm = 0.f;
		bAdvanceTurnRequested = true;
		return false;
	}

	return true;
}

bool UWS_CombatDirector::RequestEndTurn(APageCharacter* Page)
{
	if (!Page || !IsCombatActive() || !IsPageTurnActive(Page))
	{
		return false;
	}

	bAdvanceTurnRequested = true;
	UE_LOG(LogTemp, Log, TEXT("[Combat] %s requested end turn"), *GetNameSafe(Page));
	return true;
}

bool UWS_CombatDirector::RequestUseCombatAction(APageCharacter* RequestingPage, int32 SlotIndex, AActor* OptionalTarget)
{
	if (!RequestingPage || !IsCombatActive() || !RequestingPage->IsFriendly() || !IsPageTurnActive(RequestingPage))
	{
		return false;
	}

	FPageCombatActionSlot Slot;
	if (!RequestingPage->GetCombatActionSlot(SlotIndex, Slot) || Slot.ActionType == EPageCombatActionType::None)
	{
		return false;
	}

	const FCombatActionPointState* APState = CombatantActionPoints.Find(RequestingPage);
	if (!APState || APState->ActionPointsRemaining <= 0)
	{
		return false;
	}

	if (Slot.ActionType == EPageCombatActionType::EndTurn)
	{
		return RequestEndTurn(RequestingPage);
	}

	if (Slot.ActionType == EPageCombatActionType::ActiveSkill)
	{
		const FSkillDefinitionRow* SkillDef = GetActiveSkillDefinition(Slot.ActionId);
		if (!SkillDef || !SkillDef->bIsActiveCombatSkill || !RequestingPage->Skills || !RequestingPage->Skills->HasSkill(Slot.ActionId))
		{
			UE_LOG(LogTemp, Warning, TEXT("[Combat] Slot %d does not contain an owned active combat skill: %s"), SlotIndex, *Slot.ActionId.ToString());
			return false;
		}
	}

	PendingFriendlyAction = FPendingCombatActionRequest{};
	PendingFriendlyAction.RequestingPage = RequestingPage;
	PendingFriendlyAction.TargetActor = OptionalTarget;
	PendingFriendlyAction.SlotIndex = SlotIndex;
	PendingFriendlyAction.ActionType = Slot.ActionType;
	PendingFriendlyAction.ActionId = Slot.ActionId;

	UE_LOG(LogTemp, Log,
		TEXT("[Combat] %s requested slot %d action Type=%d Id=%s Target=%s"),
		*GetNameSafe(RequestingPage),
		SlotIndex,
		static_cast<int32>(Slot.ActionType),
		*Slot.ActionId.ToString(),
		*GetNameSafe(OptionalTarget));
	return true;
}



