// Fill out your copyright notice in the Description page of Project Settings.


#include "World/Settlement/WS_Population.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Simulation/SimCommandBuffer.h"
#include "Entities/Page/PageCharacter.h"
#include "Entities/Items/InventoryComponent.h"
#include "Entities/Items/EquipmentComponent.h"
#include "Entities/Page/Components/StatsComponent.h"
#include "World/Settlement/WS_Building.h"
#include "GameFramework/CharacterMovementComponent.h"

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

	EnsurePageEntityId(TargetPage);
	Stats->Revive(1.f);
	TargetPage->SetFaction(EPageFaction::Captive);
	TargetPage->CurrentJobState = FPageJobState{};
	TargetPage->SetTurnCombatState(false, false);
	if (UCharacterMovementComponent* Movement = TargetPage->GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}

	MarkCacheDirty();
	UE_LOG(LogTemp, Log, TEXT("[Population] Captured Page=%s Id=%d"), *GetNameSafe(TargetPage), TargetPage->GetPageEntityId());
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
	RefreshSettlementCapacityState();

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

void UWS_Population::RefreshSettlementCapacityState()
{
	int32 BuildingCapacity = 0;
	if (UWS_Building* BuildingSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UWS_Building>() : nullptr)
	{
		BuildingCapacity = BuildingSubsystem->GetCompletedPageCapacity();
	}

	CachedPageCapacity = FMath::Max(0, BasePageCapacity + BuildingCapacity);
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

	const float CapacityMultiplier = MutableThis->IsOverCapacity() ? MutableThis->OverCapacityWorkRateMultiplier : 1.f;
	return Page->GetSkillMultiplier(WorkId) * CapacityMultiplier;
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
		if (const UInventoryComponent* Inventory = Page->GetInventory())
		{
			PageSnapshot.InventoryStacks = Inventory->GetStacks();
		}
		if (const UEquipmentComponent* Equipment = Page->GetEquipment())
		{
			PageSnapshot.EquipmentSlots = Equipment->GetEquippedSlots();
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
		if (UInventoryComponent* Inventory = Page->GetInventory())
		{
			Inventory->SetStacks(PageSnapshot.InventoryStacks);
		}
		if (UEquipmentComponent* Equipment = Page->GetEquipment())
		{
			Equipment->SetEquippedSlots(PageSnapshot.EquipmentSlots);
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
