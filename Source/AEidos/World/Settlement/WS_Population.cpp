// Fill out your copyright notice in the Description page of Project Settings.


#include "World/Settlement/WS_Population.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Simulation/SimCommandBuffer.h"
#include "Entities/Page/PageCharacter.h"
#include "Entities/Page/Components/StatsComponent.h"

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

	bCacheDirty = false;
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

	for (int32 i = 0; i < CachedPages.Num(); ++i)
	{
		APageCharacter* Page = CachedPages[i].Get();
		if (!Page)
		{
			continue;
		}

		PlannedDeltas[i].HungerDelta = 0.05f;
		PlannedDeltas[i].FatigueDelta = 0.03f;
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

	if (Page->IsInDungeon())
	{
		return false;
	}

	return !Page->CurrentJobState.bIsActive;
}

float UWS_Population::ComputeWorkRateMultiplier_Implementation(int32 PageId, FName WorkId) const
{
	UWS_Population* MutableThis = const_cast<UWS_Population*>(this);
	MutableThis->RebuildCacheIfNeeded();

	APageCharacter* Page = MutableThis->FindPageById(PageId);
	if (!Page)
	{
		return 1.f;
	}

	return Page->GetSkillMultiplier(WorkId);
}

void UWS_Population::ApplyWorkCompletionEffects_Implementation(int32 PageId, FName WorkId)
{
	RebuildCacheIfNeeded();

	if (APageCharacter* Page = FindPageById(PageId))
	{
		Page->CurrentJobState = FPageJobState{};
	}
}

void UWS_Population::AssignPageToWork_Implementation(int32 PageId, int32 InstanceId, FName WorkId, FVector WorkLocation, int32 Priority)
{
	RebuildCacheIfNeeded();

	if (APageCharacter* Page = FindPageById(PageId))
	{
		Page->CurrentJobState.InstanceId = InstanceId;
		Page->CurrentJobState.WorkId = WorkId;
		Page->CurrentJobState.Priority = Priority;
		Page->CurrentJobState.WorkLocation = WorkLocation;
		Page->CurrentJobState.bIsActive = true;
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
