// Fill out your copyright notice in the Description page of Project Settings.


#include "World/Settlement/WS_Population.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Simulation/SimCommandBuffer.h"
#include "Entities/Page/PageCharacter.h"
#include "Entities/Items/InventoryComponent.h"
#include "Entities/Items/EquipmentComponent.h"
#include "Entities/Page/Components/SkillComponent.h"
#include "Entities/Page/Components/StatsComponent.h"
#include "World/Settlement/WS_Building.h"
#include "World/Settlement/WS_Work.h"
#include "Combat/WS_CombatDirector.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"

namespace
{
	bool TryTeleportPageToWorkSite(APageCharacter* Page, const FVector& WorkLocation)
	{
		UWorld* World = Page ? Page->GetWorld() : nullptr;
		if (!World)
		{
			return false;
		}

		static const FVector CandidateOffsets[] =
		{
			FVector::ZeroVector,
			FVector(180.f, 0.f, 0.f), FVector(-180.f, 0.f, 0.f),
			FVector(0.f, 180.f, 0.f), FVector(0.f, -180.f, 0.f),
			FVector(260.f, 260.f, 0.f), FVector(-260.f, 260.f, 0.f),
			FVector(260.f, -260.f, 0.f), FVector(-260.f, -260.f, 0.f)
		};

		for (const FVector& Offset : CandidateOffsets)
		{
			const FVector HorizontalCandidate = WorkLocation + Offset;
			FHitResult GroundHit;
			FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WorkSiteTeleport), false, Page);
			const FVector TraceStart = HorizontalCandidate + FVector(0.f, 0.f, 5000.f);
			const FVector TraceEnd = HorizontalCandidate - FVector(0.f, 0.f, 10000.f);
			if (!World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
			{
				continue;
			}

			const float CapsuleHalfHeight = Page->GetCapsuleComponent()
				? Page->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()
				: 90.f;
			FVector CandidateLocation = GroundHit.ImpactPoint + FVector(0.f, 0.f, CapsuleHalfHeight + 5.f);
			if (World->FindTeleportSpot(Page, CandidateLocation, Page->GetActorRotation()))
			{
				Page->GetCharacterMovement()->StopMovementImmediately();
				return Page->TeleportTo(CandidateLocation, Page->GetActorRotation(), false, false);
			}
		}

		return false;
	}
}

void UWS_Population::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	bCacheDirty = true;
	NextPageId = 1;

	if (!TestPageClass)
	{
		static const TCHAR* PageBPClassPath =
			TEXT("/Game/Blueprints/BP_PageCharacter.BP_PageCharacter_C");

		UClass* Loaded = LoadClass<APageCharacter>(nullptr, PageBPClassPath);

		if (Loaded)
		{
			TestPageClass = Loaded;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[Population] Failed to load test page class: %s. Fallback to native."), PageBPClassPath);
			TestPageClass = APageCharacter::StaticClass();
		}
	}
}

void UWS_Population::Deinitialize()
{
	CachedPages.Reset();
	CachedPagesById.Reset();
	PlannedDeltas.Reset();
	ActiveEmergencyRescues.Reset();
	PlannedEmergencyRescues.Reset();
	PlannedCompletedEmergencyRescues.Reset();
	PlannedCancelledEmergencyRescueTargetIds.Reset();
	Super::Deinitialize();
}

int32 UWS_Population::GetSimOrder_Implementation() const
{
	return 10;
}

void UWS_Population::RebuildCacheIfNeeded()
{
	if (!bCacheDirty)
	{
		return;
	}

	CachedPages.Reset();
	CachedPagesById.Reset();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(World, APageCharacter::StaticClass(), Found);

	for (AActor* Actor : Found)
	{
		APageCharacter* Page = Cast<APageCharacter>(Actor);
		if (!Page)
		{
			continue;
		}

		if (!Page->IsFriendly())
		{
			continue;
		}

		const int32 PageId = EnsurePageEntityId(Page);
		CachedPages.Add(Page);
		CachedPagesById.Add(PageId, Page);
		ResetPageRuntimeState(Page);
	}

	CachedPages.Sort([](const TWeakObjectPtr<APageCharacter>& A, const TWeakObjectPtr<APageCharacter>& B)
	{
		const APageCharacter* PageA = A.Get();
		const APageCharacter* PageB = B.Get();
		if (!PageA || !PageB)
		{
			return PageA != nullptr;
		}
		return PageA->GetPageEntityId() < PageB->GetPageEntityId();
	});

	PlannedDeltas.SetNum(CachedPages.Num());
	for (FPageStatsDelta& Delta : PlannedDeltas)
	{
		Delta = FPageStatsDelta{};
	}

	for (auto It = ExpeditionRosterPageIds.CreateIterator(); It; ++It)
	{
		if (!CachedPagesById.Contains(*It))
		{
			It.RemoveCurrent();
		}
	}

	bCacheDirty = false;
}

void UWS_Population::MarkCacheDirty()
{
	bCacheDirty = true;
}

const TArray<TWeakObjectPtr<APageCharacter>>& UWS_Population::GetOwnedPages() const
{
	// Faction changes (capture/recruit) must be visible before the next simulation tick.
	const_cast<UWS_Population*>(this)->RebuildCacheIfNeeded();
	return CachedPages;
}

void UWS_Population::GetCaptivePages(TArray<APageCharacter*>& OutCaptives) const
{
	OutCaptives.Reset();
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<APageCharacter> It(World); It; ++It)
	{
		if (It->IsCaptive())
		{
			OutCaptives.Add(*It);
		}
	}
}

APageCharacter* UWS_Population::FindCaptiveById(int32 PageId) const
{
	TArray<APageCharacter*> Captives;
	GetCaptivePages(Captives);
	APageCharacter* const* Found = Captives.FindByPredicate([PageId](const APageCharacter* Page)
	{
		return Page && Page->GetPageEntityId() == PageId;
	});
	return Found ? *Found : nullptr;
}

bool UWS_Population::CaptureHostilePage(APageCharacter* TargetPage)
{
	if (!TargetPage || !TargetPage->IsHostile())
	{
		return false;
	}

	UStatsComponent* Stats = TargetPage->GetStats();
	if (!Stats)
	{
		return false;
	}

	if (IsCaptiveCapacityFull())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Population] Capture refused for %s: captive capacity is full (%d/%d)"),
			*GetNameSafe(TargetPage), GetCurrentCaptiveCount(), GetCaptiveCapacity());
		return false;
	}

	EnsurePageEntityId(TargetPage);
	Stats->Revive(1.f);
	TargetPage->SetFaction(EPageFaction::Captive);
	TargetPage->SetCaptiveResistance(InitialCaptiveResistance);
	TargetPage->CurrentJobState = FPageJobState{};
	TargetPage->SetTurnCombatState(false, false);
	if (UCharacterMovementComponent* Movement = TargetPage->GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}

	MarkCacheDirty();
	UE_LOG(LogTemp, Log, TEXT("[Population] Captured Page=%s Id=%d Resistance=%.1f"), *GetNameSafe(TargetPage), TargetPage->GetPageEntityId(), TargetPage->GetCaptiveResistance());
	return true;
}

bool UWS_Population::RecruitCaptivePage(int32 PageId, FString& OutReason)
{
	APageCharacter* Captive = FindCaptiveById(PageId);
	if (!Captive)
	{
		OutReason = TEXT("Captive was not found");
		return false;
	}
	if (!Captive->IsCaptive())
	{
		OutReason = TEXT("Target is not captive");
		return false;
	}
	if (Captive->GetCaptiveResistance() > KINDA_SMALL_NUMBER)
	{
		OutReason = TEXT("Captive resistance has not been reduced yet");
		return false;
	}

	Captive->SetFaction(EPageFaction::Friendly);
	Captive->SetTurnCombatState(false, false);
	if (UCharacterMovementComponent* Movement = Captive->GetCharacterMovement())
	{
		Movement->SetMovementMode(MOVE_Walking);
	}

	if (UStatsComponent* Stats = Captive->GetStats())
	{
		const float RecruitHealth = FMath::Max(1.f, Stats->GetMaxHealth() * 0.25f);
		if (Stats->IsDead())
		{
			Stats->Revive(RecruitHealth);
		}
		else
		{
			Stats->RestoreHealth(RecruitHealth);
		}
	}

	MarkCacheDirty();
	OutReason.Reset();
	UE_LOG(LogTemp, Log, TEXT("[Population] Recruited captive Page=%s Id=%d"), *GetNameSafe(Captive), Captive->GetPageEntityId());
	return true;
}

bool UWS_Population::ToggleCaptiveRecruitment(int32 PageId, FString& OutReason)
{
	APageCharacter* Captive = FindCaptiveById(PageId);
	if (!IsRecruitableCaptive(Captive))
	{
		OutReason = TEXT("Captive must be alive, held at the settlement, and out of combat");
		return false;
	}

	if (RequestedCaptiveRecruitmentIds.Remove(PageId) > 0)
	{
		if (const FCaptiveRecruitmentOperation* Operation = ActiveCaptiveRecruitments.Find(PageId))
		{
			if (APageCharacter* Recruiter = FindPageById(Operation->RecruiterPageId);
				Recruiter && Recruiter->CurrentJobState.InstanceId == Operation->InstanceId)
			{
				Recruiter->CurrentJobState = FPageJobState{};
			}
			ActiveCaptiveRecruitments.Remove(PageId);
		}
		OutReason = TEXT("Recruitment cancelled");
		return true;
	}

	RequestedCaptiveRecruitmentIds.Add(PageId);
	OutReason = TEXT("Recruitment requested");
	return true;
}

bool UWS_Population::IsCaptiveRecruitmentRequested(int32 PageId) const
{
	return RequestedCaptiveRecruitmentIds.Contains(PageId);
}

bool UWS_Population::IsCaptiveRecruitmentActive(int32 PageId) const
{
	return ActiveCaptiveRecruitments.Contains(PageId);
}

int32 UWS_Population::GetCaptiveRecruiterPageId(int32 PageId) const
{
	if (const FCaptiveRecruitmentOperation* Operation = ActiveCaptiveRecruitments.Find(PageId))
	{
		return Operation->RecruiterPageId;
	}
	return INDEX_NONE;
}

bool UWS_Population::RescueDownedPage(APageCharacter* Rescuer, APageCharacter* DownedPage, FString& OutReason)
{
	if (!Rescuer || !DownedPage || Rescuer == DownedPage)
	{
		OutReason = TEXT("Invalid rescue target");
		return false;
	}
	if (!Rescuer->IsFriendly() || !DownedPage->IsFriendly() || Rescuer->IsInDungeon() != DownedPage->IsInDungeon())
	{
		OutReason = TEXT("Rescue requires a friendly Page in the same space");
		return false;
	}
	if (const UStatsComponent* RescuerStats = Rescuer->GetStats(); !RescuerStats || RescuerStats->IsDead() || RescuerStats->IsDowned() || RescuerStats->IsRecovering())
	{
		OutReason = TEXT("Rescuer cannot act");
		return false;
	}

	UStatsComponent* DownedStats = DownedPage->GetStats();
	if (!DownedStats || !DownedStats->IsDowned())
	{
		OutReason = TEXT("Target is not downed");
		return false;
	}
	if (FVector::DistSquared(Rescuer->GetActorLocation(), DownedPage->GetActorLocation()) > FMath::Square(RescueRangeCm))
	{
		OutReason = TEXT("Target is too far away");
		return false;
	}

	DownedStats->ReviveFromRescue(FMath::Max(1.f, DownedStats->GetMaxHealth() * RescueHealthFraction));
	DownedPage->CurrentJobState = FPageJobState{};
	DownedPage->SetTurnCombatState(false, false);
	MarkCacheDirty();
	OutReason.Reset();
	UE_LOG(LogTemp, Log, TEXT("[Population] Rescued Page=%s Id=%d"), *GetNameSafe(DownedPage), DownedPage->GetPageEntityId());
	return true;
}

int32 UWS_Population::EnsurePageEntityId(APageCharacter* Page)
{
	if (!Page)
	{
		return INDEX_NONE;
	}

	int32 PageId = Page->GetPageEntityId();
	const bool bDuplicateId = PageId > 0 && CachedPagesById.Contains(PageId) && CachedPagesById.FindRef(PageId).Get() != Page;
	if (PageId <= 0 || bDuplicateId)
	{
		PageId = NextPageId++;
		Page->SetPageEntityId(PageId);
	}

	NextPageId = FMath::Max(NextPageId, PageId + 1);
	return PageId;
}

APageCharacter* UWS_Population::FindPageById(int32 PageId) const
{
	if (const TWeakObjectPtr<APageCharacter>* Found = CachedPagesById.Find(PageId))
	{
		return Found->Get();
	}

	return nullptr;
}

bool UWS_Population::TogglePageExpeditionRoster(int32 PageId, FString& OutReason)
{
	RebuildCacheIfNeeded();
	APageCharacter* Page = FindPageById(PageId);
	if (!Page || !Page->IsFriendly() || Page->IsCaptive())
	{
		OutReason = TEXT("원정 명단에는 아군 Page만 편성할 수 있습니다.");
		return false;
	}

	const UStatsComponent* Stats = Page->GetStats();
	if (Page->IsInDungeon() || (Stats && (Stats->IsDead() || Stats->IsDowned() || Stats->IsRecovering())))
	{
		OutReason = TEXT("현재 원정에 참여할 수 없는 Page입니다.");
		return false;
	}

	if (ExpeditionRosterPageIds.Contains(PageId))
	{
		ExpeditionRosterPageIds.Remove(PageId);
		OutReason = TEXT("원정 명단에서 제외했습니다.");
	}
	else
	{
		ExpeditionRosterPageIds.Add(PageId);
		OutReason = TEXT("원정 명단에 편성했습니다.");
	}
	return true;
}

bool UWS_Population::IsPageInExpeditionRoster(int32 PageId) const
{
	return PageId != INDEX_NONE && ExpeditionRosterPageIds.Contains(PageId);
}

void UWS_Population::GetReadyExpeditionPages(TArray<APageCharacter*>& OutPages) const
{
	OutPages.Reset();
	UWS_Population* MutableThis = const_cast<UWS_Population*>(this);
	MutableThis->RebuildCacheIfNeeded();

	for (int32 PageId : ExpeditionRosterPageIds)
	{
		APageCharacter* Page = MutableThis->FindPageById(PageId);
		const UStatsComponent* Stats = Page ? Page->GetStats() : nullptr;
		if (Page && Page->IsFriendly() && !Page->IsCaptive() && !Page->IsInDungeon() &&
			(!Stats || (!Stats->IsDead() && !Stats->IsDowned() && !Stats->IsRecovering())))
		{
			OutPages.Add(Page);
		}
	}

	OutPages.Sort([](const APageCharacter& A, const APageCharacter& B)
	{
		return A.GetPageEntityId() < B.GetPageEntityId();
	});
}

int32 UWS_Population::GetExpeditionRosterCount() const
{
	return ExpeditionRosterPageIds.Num();
}

int32 UWS_Population::GetReadyExpeditionRosterCount() const
{
	TArray<APageCharacter*> ReadyPages;
	GetReadyExpeditionPages(ReadyPages);
	return ReadyPages.Num();
}

void UWS_Population::ResetPageRuntimeState(APageCharacter* Page) const
{
	if (!Page)
	{
		return;
	}

	if (!Page->CurrentJobState.bIsActive)
	{
		Page->CurrentJobState = FPageJobState{};
	}
}

void UWS_Population::SimPlan_Implementation(USimCommandBuffer* CommandBuffer, float FixedDeltaSeconds)
{
	RebuildCacheIfNeeded();
	RefreshSettlementCapacityState();
	PlanAutomaticSettlementRescues(FixedDeltaSeconds);
	PlanCaptiveRecruitment(FixedDeltaSeconds);

	for (int32 i = 0; i < CachedPages.Num(); ++i)
	{
		APageCharacter* Page = CachedPages[i].Get();
		if (!Page)
		{
			continue;
		}

		// Food is now consumed as a settlement-wide meal service. Individual
		// hunger and fatigue counters remain dormant until a later morale model.
		PlannedDeltas[i] = FPageStatsDelta{};
	}
}

int32 UWS_Population::GetCurrentPageCount() const
{
	UWS_Population* MutableThis = const_cast<UWS_Population*>(this);
	MutableThis->RebuildCacheIfNeeded();
	return MutableThis->CachedPages.Num();
}

int32 UWS_Population::GetPageCapacity() const
{
	UWS_Population* MutableThis = const_cast<UWS_Population*>(this);
	MutableThis->RebuildCacheIfNeeded();
	MutableThis->RefreshSettlementCapacityState();
	return MutableThis->CachedPageCapacity;
}

bool UWS_Population::IsOverCapacity() const
{
	UWS_Population* MutableThis = const_cast<UWS_Population*>(this);
	MutableThis->RebuildCacheIfNeeded();
	MutableThis->RefreshSettlementCapacityState();
	return MutableThis->bCachedOverCapacity;
}

int32 UWS_Population::GetCurrentCaptiveCount() const
{
	TArray<APageCharacter*> Captives;
	GetCaptivePages(Captives);
	return Captives.Num();
}

int32 UWS_Population::GetCaptiveCapacity() const
{
	int32 BuildingCapacity = 0;
	if (const UWS_Building* BuildingSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UWS_Building>() : nullptr)
	{
		BuildingCapacity = BuildingSubsystem->GetCompletedCaptiveCapacity();
	}
	return FMath::Max(0, BaseCaptiveCapacity + BuildingCapacity);
}

bool UWS_Population::IsCaptiveCapacityFull() const
{
	return GetCurrentCaptiveCount() >= GetCaptiveCapacity();
}

void UWS_Population::RefreshSettlementCapacityState()
{
	int32 BuildingCapacity = 0;
	if (UWS_Building* BuildingSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UWS_Building>() : nullptr)
	{
		BuildingCapacity = BuildingSubsystem->GetCompletedPageCapacity();
	}

	CachedPageCapacity = FMath::Max(0, BasePageCapacity + BuildingCapacity);
	CachedCaptiveCapacity = GetCaptiveCapacity();
	bCachedOverCapacity = CachedPages.Num() > CachedPageCapacity;

	for (const TWeakObjectPtr<APageCharacter>& WeakPage : CachedPages)
	{
		if (APageCharacter* Page = WeakPage.Get())
		{
			Page->SetSettlementOverCapacity(bCachedOverCapacity);
		}
	}
}

void UWS_Population::SimCommit_Implementation(USimCommandBuffer* CommandBuffer, float FixedDeltaSeconds)
{
	if (!CommandBuffer)
	{
		return;
	}

	for (int32 i = 0; i < CachedPages.Num(); ++i)
	{
		TWeakObjectPtr<APageCharacter> WeakPage = CachedPages[i];
		const FPageStatsDelta Delta = PlannedDeltas.IsValidIndex(i) ? PlannedDeltas[i] : FPageStatsDelta{};

		CommandBuffer->Enqueue([WeakPage, Delta]()
		{
			if (APageCharacter* Page = WeakPage.Get())
			{
				if (UStatsComponent* Stats = Page->GetStats())
				{
					Stats->ApplyDelta(Delta);
				}
			}
		});
	}

	TWeakObjectPtr<UWS_Population> WeakThis(this);
	for (const FEmergencyRescueOperation& PlannedRescue : PlannedEmergencyRescues)
	{
		CommandBuffer->Enqueue([WeakThis, PlannedRescue]()
		{
			UWS_Population* Population = WeakThis.Get();
			if (!Population)
			{
				return;
			}

			APageCharacter* Rescuer = Population->FindPageById(PlannedRescue.RescuerPageId);
			APageCharacter* DownedPage = Population->FindPageById(PlannedRescue.DownedPageId);
			if (!Population->IsEligibleAutomaticRescuer(Rescuer) || !Population->IsSafeAutomaticRescueTarget(DownedPage))
			{
				return;
			}

			if (UWS_Work* Work = Population->GetWorld()->GetSubsystem<UWS_Work>())
			{
				Work->InterruptPageWork(PlannedRescue.RescuerPageId);
			}

			if (!TryTeleportPageToWorkSite(Rescuer, DownedPage->GetActorLocation())
				&& FVector::DistSquared(Rescuer->GetActorLocation(), DownedPage->GetActorLocation()) > FMath::Square(Population->RescueRangeCm))
			{
				UE_LOG(LogTemp, Warning, TEXT("[Population] Automatic rescue could not find a safe position for Rescuer=%d Target=%d"),
					PlannedRescue.RescuerPageId, PlannedRescue.DownedPageId);
				return;
			}

			Rescuer->CurrentJobState.InstanceId = PlannedRescue.InstanceId;
			Rescuer->CurrentJobState.WorkId = TEXT("EmergencyRescue");
			Rescuer->CurrentJobState.Priority = TNumericLimits<int32>::Max();
			Rescuer->CurrentJobState.WorkLocation = DownedPage->GetActorLocation();
			Rescuer->CurrentJobState.bIsActive = true;
			Population->ActiveEmergencyRescues.Add(PlannedRescue.DownedPageId, PlannedRescue);
			UE_LOG(LogTemp, Log, TEXT("[Population] Automatic rescue assigned Rescuer=%d Target=%d"),
				PlannedRescue.RescuerPageId, PlannedRescue.DownedPageId);
		});
	}

	for (const FEmergencyRescueOperation& CompletedRescue : PlannedCompletedEmergencyRescues)
	{
		CommandBuffer->Enqueue([WeakThis, CompletedRescue]()
		{
			UWS_Population* Population = WeakThis.Get();
			if (!Population)
			{
				return;
			}

			APageCharacter* Rescuer = Population->FindPageById(CompletedRescue.RescuerPageId);
			APageCharacter* DownedPage = Population->FindPageById(CompletedRescue.DownedPageId);
			FString FailureReason;
			if (!Population->RescueDownedPage(Rescuer, DownedPage, FailureReason))
			{
				UE_LOG(LogTemp, Verbose, TEXT("[Population] Automatic rescue did not complete Target=%d: %s"),
					CompletedRescue.DownedPageId, *FailureReason);
			}

			if (Rescuer && Rescuer->CurrentJobState.InstanceId == CompletedRescue.InstanceId)
			{
				Rescuer->CurrentJobState = FPageJobState{};
			}
			Population->ActiveEmergencyRescues.Remove(CompletedRescue.DownedPageId);
		});
	}

	for (const int32 DownedPageId : PlannedCancelledEmergencyRescueTargetIds)
	{
		CommandBuffer->Enqueue([WeakThis, DownedPageId]()
		{
			if (UWS_Population* Population = WeakThis.Get())
			{
				if (const FEmergencyRescueOperation* Rescue = Population->ActiveEmergencyRescues.Find(DownedPageId))
				{
					if (APageCharacter* Rescuer = Population->FindPageById(Rescue->RescuerPageId);
						Rescuer && Rescuer->CurrentJobState.InstanceId == Rescue->InstanceId)
					{
						Rescuer->CurrentJobState = FPageJobState{};
					}
				}
				Population->ActiveEmergencyRescues.Remove(DownedPageId);
			}
		});
	}

	for (const FCaptiveRecruitmentOperation& PlannedRecruitment : PlannedCaptiveRecruitments)
	{
		CommandBuffer->Enqueue([WeakThis, PlannedRecruitment]()
		{
			UWS_Population* Population = WeakThis.Get();
			if (!Population)
			{
				return;
			}

			APageCharacter* Recruiter = Population->FindPageById(PlannedRecruitment.RecruiterPageId);
			APageCharacter* Captive = Population->FindCaptiveById(PlannedRecruitment.CaptivePageId);
			if (!Population->RequestedCaptiveRecruitmentIds.Contains(PlannedRecruitment.CaptivePageId)
				|| !Population->IsEligibleCaptiveRecruiter(Recruiter)
				|| !Population->IsRecruitableCaptive(Captive))
			{
				return;
			}

			if (UWS_Work* Work = Population->GetWorld()->GetSubsystem<UWS_Work>())
			{
				Work->InterruptPageWork(PlannedRecruitment.RecruiterPageId);
			}

			Recruiter->CurrentJobState.InstanceId = PlannedRecruitment.InstanceId;
			Recruiter->CurrentJobState.WorkId = TEXT("RecruitCaptive");
			Recruiter->CurrentJobState.Priority = TNumericLimits<int32>::Max() - 1;
			Recruiter->CurrentJobState.WorkLocation = Captive->GetActorLocation();
			Recruiter->CurrentJobState.bIsActive = true;
			Population->ActiveCaptiveRecruitments.Add(PlannedRecruitment.CaptivePageId, PlannedRecruitment);
			UE_LOG(LogTemp, Log, TEXT("[Population] Recruitment assigned Recruiter=%d Captive=%d"),
				PlannedRecruitment.RecruiterPageId, PlannedRecruitment.CaptivePageId);
		});
	}

	for (const FCaptiveRecruitmentOperation& CompletedRecruitment : PlannedCompletedCaptiveRecruitments)
	{
		CommandBuffer->Enqueue([WeakThis, CompletedRecruitment]()
		{
			UWS_Population* Population = WeakThis.Get();
			if (!Population)
			{
				return;
			}

			APageCharacter* Recruiter = Population->FindPageById(CompletedRecruitment.RecruiterPageId);
			APageCharacter* Captive = Population->FindCaptiveById(CompletedRecruitment.CaptivePageId);
			if (Recruiter && Recruiter->CurrentJobState.InstanceId == CompletedRecruitment.InstanceId)
			{
				Recruiter->CurrentJobState = FPageJobState{};
			}

			if (Captive)
			{
				Captive->SetCaptiveResistance(0.f);
			}

			FString FailureReason;
			if (!Population->RecruitCaptivePage(CompletedRecruitment.CaptivePageId, FailureReason))
			{
				UE_LOG(LogTemp, Warning, TEXT("[Population] Recruitment completion failed Captive=%d: %s"),
					CompletedRecruitment.CaptivePageId, *FailureReason);
			}

			Population->RequestedCaptiveRecruitmentIds.Remove(CompletedRecruitment.CaptivePageId);
			Population->ActiveCaptiveRecruitments.Remove(CompletedRecruitment.CaptivePageId);
		});
	}

	for (const int32 CaptivePageId : PlannedCancelledCaptiveRecruitmentIds)
	{
		CommandBuffer->Enqueue([WeakThis, CaptivePageId]()
		{
			if (UWS_Population* Population = WeakThis.Get())
			{
				if (const FCaptiveRecruitmentOperation* Operation = Population->ActiveCaptiveRecruitments.Find(CaptivePageId))
				{
					if (APageCharacter* Recruiter = Population->FindPageById(Operation->RecruiterPageId);
						Recruiter && Recruiter->CurrentJobState.InstanceId == Operation->InstanceId)
					{
						Recruiter->CurrentJobState = FPageJobState{};
					}
				}
				Population->ActiveCaptiveRecruitments.Remove(CaptivePageId);
			}
		});
	}
}

bool UWS_Population::IsEligibleAutomaticRescuer(const APageCharacter* Page) const
{
	if (!Page || !Page->IsFriendly() || Page->IsInDungeon() || Page->IsInTurnCombat() || Page->IsManualWorkOverrideActive())
	{
		return false;
	}

	const UStatsComponent* Stats = Page->GetStats();
	if (!Stats || Stats->IsDead() || Stats->IsDowned() || Stats->IsRecovering())
	{
		return false;
	}

	const UWS_CombatDirector* Combat = GetWorld() ? GetWorld()->GetSubsystem<UWS_CombatDirector>() : nullptr;
	return !Combat || !Combat->IsCombatant(Page);
}

bool UWS_Population::IsSafeAutomaticRescueTarget(const APageCharacter* Page) const
{
	if (!Page || !Page->IsFriendly() || Page->IsInDungeon() || Page->IsInTurnCombat())
	{
		return false;
	}

	const UStatsComponent* Stats = Page->GetStats();
	if (!Stats || !Stats->IsDowned())
	{
		return false;
	}

	const UWS_CombatDirector* Combat = GetWorld() ? GetWorld()->GetSubsystem<UWS_CombatDirector>() : nullptr;
	return !Combat || !Combat->IsCombatant(Page);
}

void UWS_Population::PlanAutomaticSettlementRescues(float FixedDeltaSeconds)
{
	PlannedEmergencyRescues.Reset();
	PlannedCompletedEmergencyRescues.Reset();
	PlannedCancelledEmergencyRescueTargetIds.Reset();

	if (!bEnableAutomaticSettlementRescue)
	{
		return;
	}

	TSet<int32> ReservedRescuers;
	for (TPair<int32, FEmergencyRescueOperation>& Pair : ActiveEmergencyRescues)
	{
		FEmergencyRescueOperation& Rescue = Pair.Value;
		APageCharacter* Rescuer = FindPageById(Rescue.RescuerPageId);
		APageCharacter* DownedPage = FindPageById(Rescue.DownedPageId);
		if (!IsEligibleAutomaticRescuer(Rescuer) || !IsSafeAutomaticRescueTarget(DownedPage))
		{
			PlannedCancelledEmergencyRescueTargetIds.Add(Rescue.DownedPageId);
			continue;
		}

		ReservedRescuers.Add(Rescue.RescuerPageId);
		Rescue.RemainingWorkSeconds = FMath::Max(0.f, Rescue.RemainingWorkSeconds - FixedDeltaSeconds);
		if (Rescue.RemainingWorkSeconds <= 0.f)
		{
			PlannedCompletedEmergencyRescues.Add(Rescue);
		}
	}

	for (const TWeakObjectPtr<APageCharacter>& WeakDownedPage : CachedPages)
	{
		APageCharacter* DownedPage = WeakDownedPage.Get();
		if (!IsSafeAutomaticRescueTarget(DownedPage))
		{
			continue;
		}

		const int32 DownedPageId = DownedPage->GetPageEntityId();
		if (ActiveEmergencyRescues.Contains(DownedPageId))
		{
			continue;
		}

		APageCharacter* BestRescuer = nullptr;
		float BestDistanceSquared = TNumericLimits<float>::Max();
		for (const TWeakObjectPtr<APageCharacter>& WeakCandidate : CachedPages)
		{
			APageCharacter* Candidate = WeakCandidate.Get();
			if (!IsEligibleAutomaticRescuer(Candidate) || Candidate == DownedPage || ReservedRescuers.Contains(Candidate->GetPageEntityId()))
			{
				continue;
			}

			const float DistanceSquared = FVector::DistSquared(Candidate->GetActorLocation(), DownedPage->GetActorLocation());
			if (DistanceSquared < BestDistanceSquared)
			{
				BestRescuer = Candidate;
				BestDistanceSquared = DistanceSquared;
			}
		}

		if (!BestRescuer)
		{
			continue;
		}

		FEmergencyRescueOperation& Rescue = PlannedEmergencyRescues.AddDefaulted_GetRef();
		Rescue.DownedPageId = DownedPageId;
		Rescue.RescuerPageId = BestRescuer->GetPageEntityId();
		Rescue.InstanceId = NextEmergencyRescueInstanceId--;
		Rescue.RemainingWorkSeconds = AutomaticRescueWorkSeconds;
		ReservedRescuers.Add(Rescue.RescuerPageId);
	}
}

bool UWS_Population::IsEligibleCaptiveRecruiter(const APageCharacter* Page) const
{
	if (!Page || !Page->IsFriendly() || Page->IsInDungeon() || Page->IsInTurnCombat() || Page->IsManualWorkOverrideActive())
	{
		return false;
	}

	const UStatsComponent* Stats = Page->GetStats();
	if (!Stats || Stats->IsDead() || Stats->IsDowned() || Stats->IsRecovering())
	{
		return false;
	}

	const UWS_CombatDirector* Combat = GetWorld() ? GetWorld()->GetSubsystem<UWS_CombatDirector>() : nullptr;
	return (!Combat || !Combat->IsCombatant(Page)) && !Page->CurrentJobState.bIsActive;
}

bool UWS_Population::IsRecruitableCaptive(const APageCharacter* Page) const
{
	if (!Page || !Page->IsCaptive() || Page->IsInDungeon() || Page->IsInTurnCombat())
	{
		return false;
	}

	const UStatsComponent* Stats = Page->GetStats();
	return Stats && !Stats->IsDead() && !Stats->IsDowned();
}

void UWS_Population::PlanCaptiveRecruitment(float FixedDeltaSeconds)
{
	PlannedCaptiveRecruitments.Reset();
	PlannedCompletedCaptiveRecruitments.Reset();
	PlannedCancelledCaptiveRecruitmentIds.Reset();

	TSet<int32> ReservedRecruiters;
	for (const TPair<int32, FCaptiveRecruitmentOperation>& Pair : ActiveCaptiveRecruitments)
	{
		const FCaptiveRecruitmentOperation& Operation = Pair.Value;
		APageCharacter* Recruiter = FindPageById(Operation.RecruiterPageId);
		APageCharacter* Captive = FindCaptiveById(Operation.CaptivePageId);
		const bool bStillAssigned = Recruiter && Recruiter->CurrentJobState.InstanceId == Operation.InstanceId;
		if (!RequestedCaptiveRecruitmentIds.Contains(Operation.CaptivePageId)
			|| !bStillAssigned
			|| !Recruiter
			|| !Captive
			|| Recruiter->IsManualWorkOverrideActive()
			|| Recruiter->IsInTurnCombat()
			|| Captive->IsInTurnCombat())
		{
			PlannedCancelledCaptiveRecruitmentIds.Add(Operation.CaptivePageId);
			continue;
		}

		ReservedRecruiters.Add(Operation.RecruiterPageId);
		const float WorkRate = CaptiveRecruitmentResistancePerSecond * Recruiter->GetSkillMultiplier(TEXT("Recruit_Captive"));
		if (Captive->GetCaptiveResistance() <= WorkRate * FixedDeltaSeconds)
		{
			PlannedCompletedCaptiveRecruitments.Add(Operation);
		}
		else
		{
			Captive->ReduceCaptiveResistance(WorkRate * FixedDeltaSeconds);
		}
	}

	for (const int32 CaptivePageId : RequestedCaptiveRecruitmentIds)
	{
		if (ActiveCaptiveRecruitments.Contains(CaptivePageId))
		{
			continue;
		}

		APageCharacter* Captive = FindCaptiveById(CaptivePageId);
		if (!IsRecruitableCaptive(Captive))
		{
			PlannedCancelledCaptiveRecruitmentIds.Add(CaptivePageId);
			continue;
		}

		APageCharacter* BestRecruiter = nullptr;
		float BestDistanceSquared = TNumericLimits<float>::Max();
		for (const TWeakObjectPtr<APageCharacter>& WeakCandidate : CachedPages)
		{
			APageCharacter* Candidate = WeakCandidate.Get();
			if (!IsEligibleCaptiveRecruiter(Candidate) || ReservedRecruiters.Contains(Candidate->GetPageEntityId()))
			{
				continue;
			}

			const float DistanceSquared = FVector::DistSquared(Candidate->GetActorLocation(), Captive->GetActorLocation());
			if (DistanceSquared < BestDistanceSquared)
			{
				BestRecruiter = Candidate;
				BestDistanceSquared = DistanceSquared;
			}
		}

		if (!BestRecruiter)
		{
			continue;
		}

		FCaptiveRecruitmentOperation& Operation = PlannedCaptiveRecruitments.AddDefaulted_GetRef();
		Operation.CaptivePageId = CaptivePageId;
		Operation.RecruiterPageId = BestRecruiter->GetPageEntityId();
		Operation.InstanceId = NextCaptiveRecruitmentInstanceId--;
		ReservedRecruiters.Add(Operation.RecruiterPageId);
	}
}

void UWS_Population::SimPost_Implementation(float FixedDeltaSeconds)
{
}

void UWS_Population::EnsureTestPageSpawned()
{
	if (!bSpawnTestPage)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	bool bAnyPageExists = false;
	for (TActorIterator<APageCharacter> It(World); It; ++It)
	{
		bAnyPageExists = true;
		break;
	}

	if (bAnyPageExists)
	{
		return;
	}

	TSubclassOf<APageCharacter> SpawnClass = TestPageClass;
	if (!SpawnClass)
	{
		SpawnClass = APageCharacter::StaticClass();
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	const int32 NumPagesToSpawn = FMath::Max(1, InitialTestPageCount);
	for (int32 Index = 0; Index < NumPagesToSpawn; ++Index)
	{
		const FVector SpawnLocation = TestSpawnLocation + (TestSpawnOffsetPerPage * Index);
		if (APageCharacter* Spawned = World->SpawnActor<APageCharacter>(SpawnClass, SpawnLocation, FRotator::ZeroRotator, Params))
		{
			Spawned->SetPageEntityId(NextPageId++);

			if (Index == 0 && bGiveStarterTestItem)
			{
				const int32 Added = Spawned->GetInventory()
					? Spawned->GetInventory()->TryAddItem(StarterTestItemId, StarterTestItemQuantity)
					: 0;
				if (Added != StarterTestItemQuantity)
				{
					UE_LOG(LogTemp, Warning, TEXT("[Population] Starter item grant failed ItemId=%s Requested=%d Added=%d. Check DT_Item and Data Registry."),
						*StarterTestItemId.ToString(), StarterTestItemQuantity, Added);
				}
				else
				{
					UE_LOG(LogTemp, Log, TEXT("[Population] Granted starter test item ItemId=%s Quantity=%d to PageId=%d"),
						*StarterTestItemId.ToString(), Added, Spawned->GetPageEntityId());
				}
			}

			if (Index == 0 && bGiveStarterBlockItems)
			{
				const int32 Added = Spawned->GetInventory()
					? Spawned->GetInventory()->TryAddItem(StarterBlockItemId, StarterBlockItemQuantity)
					: 0;
				if (Added != StarterBlockItemQuantity)
				{
					UE_LOG(LogTemp, Warning, TEXT("[Population] Starter block grant failed ItemId=%s Requested=%d Added=%d. Check DT_Item PlacedBlockClass."),
						*StarterBlockItemId.ToString(), StarterBlockItemQuantity, Added);
				}
			}

			if (Index == 0 && bEquipStarterTestTool)
			{
				UInventoryComponent* Inventory = Spawned->GetInventory();
				UEquipmentComponent* Equipment = Spawned->GetEquipment();
				const int32 AddedTool = Inventory ? Inventory->TryAddItem(StarterTestToolItemId, 1) : 0;
				const bool bEquipped = AddedTool == 1 && Equipment
					&& Equipment->EquipFromInventory(StarterTestToolItemId, EPageEquipmentSlot::RightHand);
				if (!bEquipped)
				{
					UE_LOG(LogTemp, Warning, TEXT("[Population] Starter tool equip failed ItemId=%s. Check DT_Item type, compatible slot, and tool tags."),
						*StarterTestToolItemId.ToString());
				}
				else
				{
					UE_LOG(LogTemp, Log, TEXT("[Population] Equipped starter tool ItemId=%s to RightHand PageId=%d"),
						*StarterTestToolItemId.ToString(), Spawned->GetPageEntityId());
				}
			}

			if (Index == 0 && bGiveStarterTestEquipment)
			{
				for (const FName EquipmentItemId : StarterTestEquipmentItemIds)
				{
					const int32 Added = Spawned->GetInventory() ? Spawned->GetInventory()->TryAddItem(EquipmentItemId, 1) : 0;
					if (Added != 1)
					{
						UE_LOG(LogTemp, Warning, TEXT("[Population] Starter equipment grant failed ItemId=%s. Check DT_Item."), *EquipmentItemId.ToString());
					}
				}
			}
		}
	}

	bCacheDirty = true;
}

TArray<int32> UWS_Population::GetAllPageIds_Implementation() const
{
	UWS_Population* MutableThis = const_cast<UWS_Population*>(this);
	MutableThis->RebuildCacheIfNeeded();

	TArray<int32> Result;
	MutableThis->CachedPagesById.GetKeys(Result);
	Result.Sort();
	return Result;
}

AActor* UWS_Population::GetPageActor_Implementation(int32 PageId)
{
	RebuildCacheIfNeeded();
	return FindPageById(PageId);
}

bool UWS_Population::IsPageAvailable_Implementation(int32 PageId) const
{
	UWS_Population* MutableThis = const_cast<UWS_Population*>(this);
	MutableThis->RebuildCacheIfNeeded();

	APageCharacter* Page = MutableThis->FindPageById(PageId);
	if (!Page)
	{
		return false;
	}

	if (Page->IsInDungeon() || Page->IsInTurnCombat() || Page->IsManualWorkOverrideActive())
	{
		return false;
	}
	if (const UStatsComponent* Stats = Page->GetStats(); !Stats || Stats->IsDowned() || Stats->IsRecovering() || Stats->IsDead())
	{
		return false;
	}

	return !Page->CurrentJobState.bIsActive;
}

int32 UWS_Population::GetPageWorkPriority_Implementation(int32 PageId, EWorkCategory WorkCategory) const
{
	UWS_Population* MutableThis = const_cast<UWS_Population*>(this);
	MutableThis->RebuildCacheIfNeeded();
	if (const APageCharacter* Page = MutableThis->FindPageById(PageId))
	{
		return Page->GetWorkPriority(WorkCategory);
	}
	return 0;
}

bool UWS_Population::IsPageAssignedToWork_Implementation(int32 PageId, int32 InstanceId) const
{
	UWS_Population* MutableThis = const_cast<UWS_Population*>(this);
	MutableThis->RebuildCacheIfNeeded();

	const APageCharacter* Page = MutableThis->FindPageById(PageId);
	if (!Page || Page->IsInDungeon() || Page->IsInTurnCombat())
	{
		return false;
	}
	if (const UStatsComponent* Stats = Page->GetStats(); !Stats || Stats->IsDowned() || Stats->IsRecovering() || Stats->IsDead())
	{
		return false;
	}

	return Page->CurrentJobState.bIsActive && Page->CurrentJobState.InstanceId == InstanceId;
}

float UWS_Population::ComputeWorkRateMultiplier_Implementation(int32 PageId, FName SkillId) const
{
	UWS_Population* MutableThis = const_cast<UWS_Population*>(this);
	MutableThis->RebuildCacheIfNeeded();

	APageCharacter* Page = MutableThis->FindPageById(PageId);
	if (!Page)
	{
		return 1.f;
	}

	const float CapacityMultiplier = MutableThis->IsOverCapacity() ? MutableThis->OverCapacityWorkRateMultiplier : 1.f;
	return Page->GetSkillMultiplier(SkillId) * CapacityMultiplier * Page->GetSettlementWorkRateMultiplier();
}

void UWS_Population::AwardWorkSkillXP_Implementation(int32 PageId, FName SkillId, float XPPerSecond, float FixedDeltaSeconds, float XPFactor)
{
	if (APageCharacter* Page = FindPageById(PageId))
	{
		Page->AddWorkSkillXP(SkillId, XPPerSecond, FixedDeltaSeconds, XPFactor);
	}
}

void UWS_Population::ApplyWorkCompletionEffects_Implementation(int32 PageId, FName WorkId)
{
	RebuildCacheIfNeeded();

	if (APageCharacter* Page = FindPageById(PageId))
	{
		Page->CurrentJobState = FPageJobState{};
	}
}

void UWS_Population::AssignPageToWork_Implementation(int32 PageId, int32 InstanceId, FName WorkId, FVector WorkLocation, int32 Priority, bool bTeleportToWorkSite)
{
	RebuildCacheIfNeeded();

	if (APageCharacter* Page = FindPageById(PageId))
	{
		Page->CurrentJobState.InstanceId = InstanceId;
		Page->CurrentJobState.WorkId = WorkId;
		Page->CurrentJobState.Priority = Priority;
		Page->CurrentJobState.WorkLocation = WorkLocation;
		Page->CurrentJobState.bIsActive = true;

		if (bTeleportToWorkSite && !TryTeleportPageToWorkSite(Page, WorkLocation))
		{
			UE_LOG(LogTemp, Verbose, TEXT("[Work] Page=%d has no free work-site teleport spot for %s; working in place"), PageId, *WorkId.ToString());
		}
	}
}

void UWS_Population::ClearPageWorkAssignment_Implementation(int32 PageId, int32 InstanceId)
{
	RebuildCacheIfNeeded();

	if (APageCharacter* Page = FindPageById(PageId))
	{
		if (InstanceId == INDEX_NONE || Page->CurrentJobState.InstanceId == InstanceId)
		{
			Page->CurrentJobState = FPageJobState{};
		}
	}
}

void UWS_Population::WriteToSnapshot_Implementation(FEidosWorldSnapshot& InOutSnapshot) const
{
	UWS_Population* MutableThis = const_cast<UWS_Population*>(this);
	MutableThis->RebuildCacheIfNeeded();

	InOutSnapshot.Population.NextPageId = NextPageId;
	InOutSnapshot.Population.Pages.Reset();
	InOutSnapshot.Population.ExpeditionRosterPageIds.Reset();
	for (int32 PageId : ExpeditionRosterPageIds)
	{
		InOutSnapshot.Population.ExpeditionRosterPageIds.Add(PageId);
	}
	InOutSnapshot.Population.ExpeditionRosterPageIds.Sort();

	for (const TWeakObjectPtr<APageCharacter>& WeakPage : CachedPages)
	{
		const APageCharacter* Page = WeakPage.Get();
		if (!Page)
		{
			continue;
		}

		FEidosPageSnapshot PageSnapshot;
		PageSnapshot.PageId = Page->GetPageEntityId();
		PageSnapshot.Transform = Page->GetActorTransform();
		PageSnapshot.PageClass = FSoftClassPath(Page->GetClass());
		if (const UInventoryComponent* Inventory = Page->GetInventory())
		{
			PageSnapshot.InventoryStacks = Inventory->GetStacks();
		}
		if (const UEquipmentComponent* Equipment = Page->GetEquipment())
		{
		PageSnapshot.EquipmentSlots = Equipment->GetEquippedSlots();
		}
		PageSnapshot.WorkPriorities = Page->GetWorkPriorities();
		if (const USkillComponent* Skills = Page->Skills)
		{
			PageSnapshot.SkillStates = Skills->GetAllSkillStates();
		}
		InOutSnapshot.Population.Pages.Add(PageSnapshot);
	}
}

void UWS_Population::ApplySnapshot_Implementation(const FEidosWorldSnapshot& Snapshot)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	NextPageId = FMath::Max(1, Snapshot.Population.NextPageId);
	ExpeditionRosterPageIds.Reset();
	for (int32 PageId : Snapshot.Population.ExpeditionRosterPageIds)
	{
		if (PageId != INDEX_NONE)
		{
			ExpeditionRosterPageIds.Add(PageId);
		}
	}
	bCacheDirty = true;
	RebuildCacheIfNeeded();

	TMap<int32, APageCharacter*> ExistingById;
	TArray<APageCharacter*> UnassignedExisting;
	for (const TWeakObjectPtr<APageCharacter>& WeakPage : CachedPages)
	{
		if (APageCharacter* Page = WeakPage.Get())
		{
			Page->CurrentJobState = FPageJobState{};
			if (Page->GetPageEntityId() > 0)
			{
				ExistingById.Add(Page->GetPageEntityId(), Page);
			}
			else
			{
				UnassignedExisting.Add(Page);
			}
		}
	}

	TSet<APageCharacter*> MatchedPages;
	for (const FEidosPageSnapshot& PageSnapshot : Snapshot.Population.Pages)
	{
		APageCharacter* Page = ExistingById.FindRef(PageSnapshot.PageId);
		if (!Page && UnassignedExisting.Num() > 0)
		{
			Page = UnassignedExisting[0];
			UnassignedExisting.RemoveAt(0);
		}

		if (!Page)
		{
			UClass* SpawnClass = PageSnapshot.PageClass.TryLoadClass<APageCharacter>();
			if (!SpawnClass)
			{
				SpawnClass = TestPageClass ? *TestPageClass : APageCharacter::StaticClass();
			}

			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			Page = World->SpawnActor<APageCharacter>(
				SpawnClass,
				PageSnapshot.Transform.GetLocation(),
				PageSnapshot.Transform.Rotator(),
				Params);
		}

		if (!Page)
		{
			continue;
		}

		Page->SetPageEntityId(PageSnapshot.PageId);
		Page->SetActorTransform(PageSnapshot.Transform);
		Page->CurrentJobState = FPageJobState{};
		Page->SetWorkPriorities(PageSnapshot.WorkPriorities);
		if (UInventoryComponent* Inventory = Page->GetInventory())
		{
			Inventory->SetStacks(PageSnapshot.InventoryStacks);
		}
		if (UEquipmentComponent* Equipment = Page->GetEquipment())
		{
			Equipment->SetEquippedSlots(PageSnapshot.EquipmentSlots);
		}
		if (USkillComponent* Skills = Page->Skills)
		{
			Skills->SetAllSkillStates(PageSnapshot.SkillStates);
		}
		MatchedPages.Add(Page);
		NextPageId = FMath::Max(NextPageId, PageSnapshot.PageId + 1);
	}

	for (const TWeakObjectPtr<APageCharacter>& WeakPage : CachedPages)
	{
		if (APageCharacter* Page = WeakPage.Get())
		{
			if (!MatchedPages.Contains(Page))
			{
				Page->Destroy();
			}
		}
	}

	bCacheDirty = true;
	RebuildCacheIfNeeded();
}
