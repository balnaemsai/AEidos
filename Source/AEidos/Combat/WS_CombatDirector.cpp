// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/WS_CombatDirector.h"

#include "Data/Definitions/SkillDefinitionRow.h"
#include "Data/GIS_DataRegistry.h"
#include "Engine/GameInstance.h"
#include "Entities/Page/PageCharacter.h"
#include "Entities/Page/Components/StatsComponent.h"
#include "Framework/EidosPlayerController.h"
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
		if (!Stats || Stats->IsDead())
		{
			continue;
		}

		Pages.Add(Page);
	}

	return Pages;
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
			TArray<APageCharacter*> Combatants;
			for (APageCharacter* Candidate : Pages)
			{
				if (Candidate && Candidate->IsInDungeon() == Page->IsInDungeon())
				{
					Combatants.Add(Candidate);
				}
			}

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

	return false;
}

void UWS_CombatDirector::StartEncounter(const TArray<APageCharacter*>& Combatants, bool bCombatSpaceIsDungeon)
{
	EndEncounter(TEXT("RestartEncounter"));

	ActiveEncounter = FCombatEncounterRuntime{};
	ActiveEncounter.EncounterId = NextEncounterId++;
	ActiveEncounter.RoundIndex = 1;
	ActiveEncounter.ActiveTurnIndex = 0;
	ActiveEncounter.TurnTimeRemaining = FriendlyTurnDurationSeconds;
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
		APState.MaxActionPoints = FMath::Max(1, ActionPointsPerTurn);
		APState.ActionPointsRemaining = 0;
		APState.MovementProgressCm = 0.f;
	}

	RefreshEncounterState();
	BeginTurnForCombatant(GetActiveCombatant());
	FocusActiveFriendlyPage();
}

void UWS_CombatDirector::RebuildCombatantsFromWorld(const TArray<APageCharacter*>& Pages)
{
	if (!IsCombatActive())
	{
		return;
	}

	TArray<TWeakObjectPtr<APageCharacter>> RebuiltCombatants;
	for (APageCharacter* Page : Pages)
	{
		if (Page && Page->IsInDungeon() == ActiveEncounter.bCombatSpaceIsDungeon)
		{
			RebuiltCombatants.Add(Page);
		}
	}

	ActiveEncounter.Combatants = MoveTemp(RebuiltCombatants);
	if (ActiveEncounter.Combatants.Num() > 0)
	{
		ActiveEncounter.ActiveTurnIndex = ActiveEncounter.ActiveTurnIndex % ActiveEncounter.Combatants.Num();
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
			else
			{
				++HostileCount;
			}
		}
	}

	if (FriendlyCount == 0 || HostileCount == 0)
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
	APState.MaxActionPoints = FMath::Max(1, ActionPointsPerTurn);
	APState.ActionPointsRemaining = APState.MaxActionPoints;
	APState.MovementProgressCm = 0.f;

	bAdvanceTurnRequested = false;
	PendingFriendlyAction = FPendingCombatActionRequest{};
	UpdateCombatantTurnFlags(ActivePage);
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

	APageCharacter* TargetPage = Request.TargetPage.Get();
	if (SkillDef->bRequiresTarget)
	{
		if (!TargetPage)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Combat] Skill %s requires a target"), *Request.ActionId.ToString());
			return false;
		}

		if (SkillDef->bTargetHostileOnly && !ActivePage->IsHostileTo(TargetPage))
		{
			return false;
		}

		if (ActivePage->IsInDungeon() != TargetPage->IsInDungeon())
		{
			return false;
		}

		const float Distance = FVector::Distance(ActivePage->GetActorLocation(), TargetPage->GetActorLocation());
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
	const TWeakObjectPtr<APageCharacter> WeakTarget = TargetPage;
	const float Damage = SkillDef->CombatDamageAmount;
	const FName SkillId = Request.ActionId;

	Cmd->Enqueue([WeakAttacker, WeakTarget, Damage, SkillId]()
	{
		APageCharacter* Attacker = WeakAttacker.Get();
		if (!Attacker)
		{
			return;
		}

		if (APageCharacter* Target = WeakTarget.Get())
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
					UE_LOG(LogTemp, Warning, TEXT("[Combat] %s died"), *GetNameSafe(Target));
					Target->Destroy();
				}
			}
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

	ActiveEncounter.ActiveTurnIndex = (ActiveEncounter.ActiveTurnIndex + 1) % ActiveEncounter.Combatants.Num();
	if (ActiveEncounter.ActiveTurnIndex == 0)
	{
		++ActiveEncounter.RoundIndex;
	}

	ActiveEncounter.TurnTimeRemaining = FriendlyTurnDurationSeconds;
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

	if (bAdvanceTurnRequested)
	{
		AdvanceTurn();
		return;
	}

	if (ActivePage->IsFriendly())
	{
		ExecutePendingFriendlyAction(Cmd, ActivePage, Pages);
		if (bAdvanceTurnRequested)
		{
			AdvanceTurn();
		}
		return;
	}

	APageCharacter* ClosestEnemy = FindClosestHostileTarget(ActivePage, Pages, DetectionRangeCm);
	if (!ClosestEnemy)
	{
		AdvanceTurn();
		return;
	}

	const float Distance = FVector::Distance(ActivePage->GetActorLocation(), ClosestEnemy->GetActorLocation());
	if (Distance <= AttackRangeCm)
	{
		if (!TrySpendActionPoints(ActivePage, 1, TEXT("EnemyAttack")))
		{
			AdvanceTurn();
			return;
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
					Target->Destroy();
				}
			}
		});

		if (bAdvanceTurnRequested)
		{
			AdvanceTurn();
		}
		return;
	}

	if (!TrySpendActionPoints(ActivePage, 1, TEXT("EnemyMove")))
	{
		AdvanceTurn();
		return;
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

bool UWS_CombatDirector::RequestUseCombatAction(APageCharacter* RequestingPage, int32 SlotIndex, APageCharacter* OptionalTarget)
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
		if (!SkillDef || !SkillDef->bIsActiveCombatSkill)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Combat] Slot %d skill is not a valid active combat skill: %s"), SlotIndex, *Slot.ActionId.ToString());
			return false;
		}
	}

	PendingFriendlyAction = FPendingCombatActionRequest{};
	PendingFriendlyAction.RequestingPage = RequestingPage;
	PendingFriendlyAction.TargetPage = OptionalTarget;
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
