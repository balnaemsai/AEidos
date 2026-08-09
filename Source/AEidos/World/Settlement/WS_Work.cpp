// Fill out your copyright notice in the Description page of Project Settings.


#include "World/Settlement/WS_Work.h"

#include "Data/GIS_DataRegistry.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Simulation/SimCommandBuffer.h"
#include "World/Settlement/EidosAccessInterface.h"
#include "World/Settlement/WS_Economy.h"
#include "World/Settlement/WS_ItemStorage.h"
#include "World/Settlement/WS_Population.h"

void UWS_Work::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Collection.InitializeDependency<UWS_Economy>();
	Collection.InitializeDependency<UWS_Population>();
	Collection.InitializeDependency<UWS_ItemStorage>();
	EconomyObj = GetWorld()->GetSubsystem<UWS_Economy>();
	PopulationObj = GetWorld()->GetSubsystem<UWS_Population>();
	ItemStorage = GetWorld()->GetSubsystem<UWS_ItemStorage>();

	if (EconomyObj && !EconomyObj->GetClass()->ImplementsInterface(UEidosEconomyAccess::StaticClass()))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Work] Economy subsystem does not implement IEidosEconomyAccess"));
		EconomyObj = nullptr;
	}
	if (PopulationObj && !PopulationObj->GetClass()->ImplementsInterface(UEidosPopulationAccess::StaticClass()))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Work] Population subsystem does not implement IEidosPopulationAccess"));
		PopulationObj = nullptr;
	}
}

void UWS_Work::SimPlan_Implementation(USimCommandBuffer* CommandBuffer, float FixedDeltaSeconds)
{
	PlannedNewInstances.Reset();
	PlannedCompletedInstances.Reset();
	PlannedPageAssignments.Reset();

	TrySpawnInstancesFromQueue();
	UpdateAssignments(FixedDeltaSeconds);
	ProgressInstances(FixedDeltaSeconds);
}

void UWS_Work::SimCommit_Implementation(USimCommandBuffer* CommandBuffer, float FixedDeltaSeconds)
{
	if (!CommandBuffer)
	{
		return;
	}

	TWeakObjectPtr<UWS_Work> WeakThis(this);

	for (const FWorkInstance& Planned : PlannedNewInstances)
	{
		const FWorkDefinitionRow* Def = FindDef(Planned.WorkId);
		if (!Def)
		{
			continue;
		}

		const FWorkInstance Inst = Planned;
		const TArray<FWorkCost> Costs = Def->Costs;

		CommandBuffer->Enqueue([WeakThis, Inst, Costs]()
		{
			if (UWS_Work* Work = WeakThis.Get())
			{
				Work->ActiveInstances.Add(Inst);
				Work->BroadcastRequestState(Inst.RequestId, EWorkRequestLifecycleState::Active);
				Work->ConsumeWorkCosts(Costs);
			}
		});
	}

	for (const FPlannedPageAssignment& Assignment : PlannedPageAssignments)
	{
		CommandBuffer->Enqueue([WeakThis, Assignment]()
		{
			if (UWS_Work* Work = WeakThis.Get())
			{
				if (Work->PopulationObj)
				{
					IEidosPopulationAccess::Execute_AssignPageToWork(
						Work->PopulationObj,
						Assignment.PageId,
						Assignment.InstanceId,
						Assignment.WorkId,
						Assignment.WorkLocation,
						Assignment.Priority);
				}

				// Worker membership changes during planning, but the popup reads the
				// committed instance state. Notify it after the assignment is applied.
				if (const FWorkInstance* Instance = Work->ActiveInstances.FindByPredicate([&Assignment](const FWorkInstance& Candidate)
				{
					return Candidate.InstanceId == Assignment.InstanceId;
				}))
				{
					Work->BroadcastRequestState(Instance->RequestId, EWorkRequestLifecycleState::Active);
				}
			}
		});
	}

	for (const FWorkInstance& Completed : PlannedCompletedInstances)
	{
		CommandBuffer->Enqueue([WeakThis, Completed]()
		{
			if (UWS_Work* Work = WeakThis.Get())
			{
				Work->ActiveInstances.RemoveAll([&](const FWorkInstance& Instance)
				{
					return Instance.InstanceId == Completed.InstanceId;
				});

				Work->HandleInstanceCompleted(Completed);
			}
		});
	}

	const int32 RemovedRequests = Queue.RemoveAll([](const FWorkRequest& Request)
	{
		return Request.Mode == EWorkRequestMode::Count && Request.RemainingCount <= 0;
	});
	if (RemovedRequests > 0)
	{
		// The queue is now authoritative, so refresh any order-list UI after removals.
		OnWorkRequestStateChanged.Broadcast(INDEX_NONE, EWorkRequestLifecycleState::Cancelled);
	}
}

int32 UWS_Work::AddWorkRequest(const FWorkRequest& InReq)
{
	FWorkRequest Req = InReq;
	Req.RequestId = NextRequestId++;

	if (Req.Mode == EWorkRequestMode::Count && Req.RemainingCount <= 0)
	{
		Req.RemainingCount = 1;
	}

	Queue.Add(Req);
	Queue.Sort([](const FWorkRequest& A, const FWorkRequest& B)
	{
		if (A.Priority != B.Priority)
		{
			return A.Priority > B.Priority;
		}
		return A.RequestId < B.RequestId;
	});

	UE_LOG(LogTemp, Log, TEXT("[Work] AddWorkRequest id=%d work=%s mode=%d prio=%d"), Req.RequestId, *Req.WorkId.ToString(), (int32)Req.Mode, Req.Priority);
	BroadcastRequestState(Req.RequestId, EWorkRequestLifecycleState::Queued);
	return Req.RequestId;
}

int32 UWS_Work::QueueWorkById(FName WorkId, int32 Quantity, int32 Priority)
{
	if (WorkId.IsNone() || Quantity <= 0)
	{
		return INDEX_NONE;
	}

	const FWorkDefinitionRow* Def = FindDef(WorkId);
	if (!Def || !CanReserveWorkCosts(Def->Costs, Quantity) || !CanStoreWorkRewards(Def->Rewards))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Work] QueueWorkById rejected WorkId=%s: insufficient unreserved costs or storage"), *WorkId.ToString());
		return INDEX_NONE;
	}

	FWorkRequest Request;
	Request.WorkId = WorkId;
	Request.Mode = EWorkRequestMode::Count;
	Request.RemainingCount = Quantity;
	Request.Priority = Priority;
	return AddWorkRequest(Request);
}

TArray<FWorkOrderView> UWS_Work::GetCraftableWorkOrders() const
{
	TArray<FWorkOrderView> Views;
	for (const TPair<FName, FWorkDefinitionRow>& Pair : WorkDefs)
	{
		const FWorkDefinitionRow& Def = Pair.Value;
		if (Def.WorkCategory != EWorkCategory::Craft)
		{
			continue;
		}
		if (!Def.Rewards.ContainsByPredicate([](const FWorkReward& Reward) { return !Reward.ItemId.IsNone(); }))
		{
			continue;
		}

		FWorkOrderView& View = Views.AddDefaulted_GetRef();
		View.WorkId = Def.WorkId;
		View.DisplayName = Def.DisplayName;
		View.Costs = Def.Costs;
		View.Rewards = Def.Rewards;
		View.bCanQueue = CanReserveWorkCosts(Def.Costs, 1) && CanStoreWorkRewards(Def.Rewards);
		for (const FWorkRequest& Request : Queue)
		{
			if (Request.WorkId == Def.WorkId)
			{
				View.QueuedCount += FMath::Max(0, Request.RemainingCount);
				View.CancelRequestId = FMath::Max(View.CancelRequestId, Request.RequestId);
			}
		}
		for (const FWorkInstance& Instance : ActiveInstances)
		{
			if (Instance.WorkId == Def.WorkId)
			{
				++View.ActiveCount;
				View.ActiveWorkerCount += Instance.Workers.Num();
				View.ActiveMaxWorkers += Instance.MaxWorkers;
				View.ActiveProgress += Instance.Progress;
				View.ActiveTotalWork += Instance.TotalWork;
				View.CancelRequestId = FMath::Max(View.CancelRequestId, Instance.RequestId);
			}
		}
		View.bCanCancel = View.CancelRequestId != INDEX_NONE;
	}
	Views.Sort([](const FWorkOrderView& A, const FWorkOrderView& B) { return A.WorkId.LexicalLess(B.WorkId); });
	return Views;
}

bool UWS_Work::GetWorkOrderView(FName WorkId, FWorkOrderView& OutView) const
{
	const FWorkDefinitionRow* Def = FindDef(WorkId);
	if (!Def)
	{
		return false;
	}

	OutView = FWorkOrderView();
	OutView.WorkId = Def->WorkId;
	OutView.DisplayName = Def->DisplayName;
	OutView.Costs = Def->Costs;
	OutView.Rewards = Def->Rewards;
	OutView.bCanQueue = CanReserveWorkCosts(Def->Costs, 1) && CanStoreWorkRewards(Def->Rewards);
	for (const FWorkRequest& Request : Queue)
	{
		if (Request.WorkId == Def->WorkId)
		{
			OutView.QueuedCount += FMath::Max(0, Request.RemainingCount);
			OutView.CancelRequestId = FMath::Max(OutView.CancelRequestId, Request.RequestId);
		}
	}
	for (const FWorkInstance& Instance : ActiveInstances)
	{
		if (Instance.WorkId == Def->WorkId)
		{
			++OutView.ActiveCount;
			OutView.ActiveWorkerCount += Instance.Workers.Num();
			OutView.ActiveMaxWorkers += Instance.MaxWorkers;
			OutView.ActiveProgress += Instance.Progress;
			OutView.ActiveTotalWork += Instance.TotalWork;
			OutView.CancelRequestId = FMath::Max(OutView.CancelRequestId, Instance.RequestId);
		}
	}
	OutView.bCanCancel = OutView.CancelRequestId != INDEX_NONE;
	return true;
}

void UWS_Work::GetOutstandingRequestIdsForWork(FName WorkId, TArray<int32>& OutRequestIds) const
{
	OutRequestIds.Reset();
	for (const FWorkRequest& Request : Queue)
	{
		if (Request.WorkId == WorkId)
		{
			OutRequestIds.AddUnique(Request.RequestId);
		}
	}
	for (const FWorkInstance& Instance : ActiveInstances)
	{
		if (Instance.WorkId == WorkId)
		{
			OutRequestIds.AddUnique(Instance.RequestId);
		}
	}
	OutRequestIds.Sort();
}

bool UWS_Work::IsRequestSatisfied(const FWorkRequest& Req) const
{
	if (!EconomyObj)
	{
		return false;
	}

	if (Req.Mode == EWorkRequestMode::Until)
	{
		const int32 Current = IEidosEconomyAccess::Execute_GetResourceAmount(EconomyObj, Req.UntilResourceId);
		return Current >= Req.UntilTargetAmount;
	}

	return false;
}

bool UWS_Work::CanAffordWorkCosts(const TArray<FWorkCost>& Costs) const
{
	if (!EconomyObj || !ItemStorage)
	{
		return false;
	}

	TMap<FName, int32> RequiredResources;
	TMap<FName, int32> RequiredItems;
	for (const FWorkCost& Cost : Costs)
	{
		if (Cost.Amount <= 0)
		{
			continue;
		}
		if (!Cost.ResourceId.IsNone() && !Cost.ItemId.IsNone())
		{
			UE_LOG(LogTemp, Warning, TEXT("[Work] Invalid cost with both ResourceId and ItemId"));
			return false;
		}
		if (!Cost.ResourceId.IsNone()) RequiredResources.FindOrAdd(Cost.ResourceId) += Cost.Amount;
		if (!Cost.ItemId.IsNone()) RequiredItems.FindOrAdd(Cost.ItemId) += Cost.Amount;
	}

	for (const TPair<FName, int32>& Pair : RequiredResources)
	{
		if (IEidosEconomyAccess::Execute_GetResourceAmount(EconomyObj, Pair.Key) < Pair.Value)
		{
			return false;
		}
	}
	for (const TPair<FName, int32>& Pair : RequiredItems)
	{
		if (ItemStorage->GetStoredItemAmount(Pair.Key) < Pair.Value)
		{
			return false;
		}
	}

	return true;
}

bool UWS_Work::CanReserveWorkCosts(const TArray<FWorkCost>& Costs, int32 Quantity) const
{
	if (!EconomyObj || !ItemStorage || Quantity <= 0)
	{
		return false;
	}

	TMap<FName, int32> RequiredResources;
	TMap<FName, int32> RequiredItems;
	const auto AddCosts = [&RequiredResources, &RequiredItems](const TArray<FWorkCost>& SourceCosts, int32 Multiplier)
	{
		for (const FWorkCost& Cost : SourceCosts)
		{
			if (Cost.Amount <= 0 || (!Cost.ResourceId.IsNone() && !Cost.ItemId.IsNone()))
			{
				continue;
			}
			if (!Cost.ResourceId.IsNone()) RequiredResources.FindOrAdd(Cost.ResourceId) += Cost.Amount * Multiplier;
			if (!Cost.ItemId.IsNone()) RequiredItems.FindOrAdd(Cost.ItemId) += Cost.Amount * Multiplier;
		}
	};

	// Pending requests have not paid their costs yet, so they reserve them for later execution.
	for (const FWorkRequest& Request : Queue)
	{
		if (Request.Mode != EWorkRequestMode::Count || Request.RemainingCount <= 0)
		{
			continue;
		}
		if (const FWorkDefinitionRow* QueuedDef = FindDef(Request.WorkId))
		{
			AddCosts(QueuedDef->Costs, Request.RemainingCount);
		}
	}
	AddCosts(Costs, Quantity);

	for (const TPair<FName, int32>& Pair : RequiredResources)
	{
		if (IEidosEconomyAccess::Execute_GetResourceAmount(EconomyObj, Pair.Key) < Pair.Value)
		{
			return false;
		}
	}
	for (const TPair<FName, int32>& Pair : RequiredItems)
	{
		if (ItemStorage->GetStoredItemAmount(Pair.Key) < Pair.Value)
		{
			return false;
		}
	}

	return true;
}

bool UWS_Work::CanStoreWorkRewards(const TArray<FWorkReward>& Rewards) const
{
	if (!ItemStorage)
	{
		return false;
	}

	TArray<FItemStack> ItemRewards;
	for (const FWorkReward& Reward : Rewards)
	{
		if (Reward.Amount <= 0)
		{
			continue;
		}
		if (!Reward.ResourceId.IsNone() && !Reward.ItemId.IsNone())
		{
			UE_LOG(LogTemp, Warning, TEXT("[Work] Invalid reward with both ResourceId and ItemId"));
			return false;
		}
		if (!Reward.ItemId.IsNone())
		{
			ItemRewards.Add({Reward.ItemId, Reward.Amount, 0.f});
		}
	}

	return ItemStorage->CanStoreItemStacks(ItemRewards);
}

void UWS_Work::ConsumeWorkCosts(const TArray<FWorkCost>& Costs)
{
	TArray<FWorkCost> ResourceCosts;
	for (const FWorkCost& Cost : Costs)
	{
		if (!Cost.ResourceId.IsNone())
		{
			ResourceCosts.Add(Cost);
		}
		else if (!Cost.ItemId.IsNone() && ItemStorage)
		{
			float RemovedQuality = 0.f;
			ItemStorage->TryTakeStoredItem(Cost.ItemId, Cost.Amount, RemovedQuality);
		}
	}
	if (EconomyObj && ResourceCosts.Num() > 0)
	{
		IEidosEconomyAccess::Execute_ConsumeCosts(EconomyObj, ResourceCosts);
	}
}

void UWS_Work::GrantWorkRewards(const TArray<FWorkReward>& Rewards)
{
	TArray<FWorkReward> ResourceRewards;
	for (const FWorkReward& Reward : Rewards)
	{
		if (!Reward.ResourceId.IsNone())
		{
			ResourceRewards.Add(Reward);
		}
		else if (!Reward.ItemId.IsNone() && ItemStorage)
		{
			ItemStorage->TryStoreItem(Reward.ItemId, Reward.Amount);
		}
	}
	if (EconomyObj && ResourceRewards.Num() > 0)
	{
		IEidosEconomyAccess::Execute_GrantRewards(EconomyObj, ResourceRewards);
	}
}

void UWS_Work::TrySpawnInstancesFromQueue()
{
	if (!EconomyObj)
	{
		return;
	}

	for (int32 Index = 0; Index < Queue.Num(); ++Index)
	{
		FWorkRequest& Req = Queue[Index];

		if (Req.Mode == EWorkRequestMode::Count && Req.RemainingCount <= 0)
		{
			continue;
		}
		if (Req.Mode == EWorkRequestMode::Until && IsRequestSatisfied(Req))
		{
			continue;
		}
		if (HasActiveInstanceForRequest(Req.RequestId))
		{
			continue;
		}

		const FWorkDefinitionRow* Def = FindDef(Req.WorkId);
		if (!Def)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Work] Missing definition for request %s"), *Req.WorkId.ToString());
			if (Req.Mode == EWorkRequestMode::Count)
			{
				Req.RemainingCount = 0;
			}
			BroadcastRequestState(Req.RequestId, EWorkRequestLifecycleState::Failed);
			continue;
		}

		if (!CanAffordWorkCosts(Def->Costs))
		{
			// Count-mode recipe orders are explicit player commands, not standing orders.
			// Do not leave an unaffordable request stuck in the queue forever.
			if (Req.Mode == EWorkRequestMode::Count)
			{
				UE_LOG(LogTemp, Warning, TEXT("[Work] Removing unaffordable queued order id=%d work=%s"), Req.RequestId, *Req.WorkId.ToString());
				Req.RemainingCount = 0;
			}
			continue;
		}

		FWorkInstance Inst;
		Inst.InstanceId = NextInstanceId++;
		Inst.RequestId = Req.RequestId;
		Inst.WorkId = Req.WorkId;
		Inst.TotalWork = Def->TotalWork;
		Inst.Progress = 0.f;
		Inst.MaxWorkers = Def->MaxWorkers;
		Inst.SiteLocation = ResolveSiteLocationForWork(*Def);
		PlannedNewInstances.Add(Inst);

		if (Req.Mode == EWorkRequestMode::Count)
		{
			Req.RemainingCount -= 1;
		}

		break;
	}
}

void UWS_Work::UpdateAssignments(float FixedDeltaSeconds)
{
	if (!PopulationObj)
	{
		return;
	}

	const TArray<int32> PageIds = IEidosPopulationAccess::Execute_GetAllPageIds(PopulationObj);
	TSet<int32> ReservedPageIds;
	for (const FWorkInstance& Inst : ActiveInstances)
	{
		for (int32 WorkerId : Inst.Workers)
		{
			ReservedPageIds.Add(WorkerId);
		}
	}

	for (FWorkInstance& Inst : ActiveInstances)
	{
		const FWorkDefinitionRow* Def = FindDef(Inst.WorkId);
		if (!Def)
		{
			continue;
		}

		while (Inst.Workers.Num() < Inst.MaxWorkers)
		{
			int32 SelectedPageId = INDEX_NONE;

			for (int32 PageId : PageIds)
			{
				if (ReservedPageIds.Contains(PageId))
				{
					continue;
				}
				if (!IEidosPopulationAccess::Execute_IsPageAvailable(PopulationObj, PageId))
				{
					continue;
				}

				SelectedPageId = PageId;
				break;
			}

			if (SelectedPageId == INDEX_NONE)
			{
				break;
			}

			Inst.Workers.Add(SelectedPageId);
			ReservedPageIds.Add(SelectedPageId);

			FPlannedPageAssignment Assignment;
			Assignment.PageId = SelectedPageId;
			Assignment.InstanceId = Inst.InstanceId;
			Assignment.WorkId = Inst.WorkId;
			Assignment.WorkLocation = Inst.SiteLocation;
			Assignment.Priority = 0;
			PlannedPageAssignments.Add(Assignment);
		}
	}
}

void UWS_Work::ProgressInstances(float FixedDeltaSeconds)
{
	if (!PopulationObj)
	{
		return;
	}

	for (FWorkInstance& Inst : ActiveInstances)
	{
		const FWorkDefinitionRow* Def = FindDef(Inst.WorkId);
		if (!Def)
		{
			continue;
		}

		float TotalRateThisTick = 0.f;
		if (Inst.MaxWorkers == 0)
		{
			TotalRateThisTick = Def->BaseWorkRate;
		}
		else
		{
			for (int32 PageId : Inst.Workers)
			{
				const float Multiplier = IEidosPopulationAccess::Execute_ComputeWorkRateMultiplier(PopulationObj, PageId, Inst.WorkId);
				TotalRateThisTick += Def->BaseWorkRate * Multiplier;
			}
		}

		Inst.Progress += TotalRateThisTick * FixedDeltaSeconds;
		if (Inst.Progress >= Inst.TotalWork && CanStoreWorkRewards(Def->Rewards))
		{
			const bool bAlreadyQueued = PlannedCompletedInstances.ContainsByPredicate([&](const FWorkInstance& Existing)
			{
				return Existing.InstanceId == Inst.InstanceId;
			});
			if (!bAlreadyQueued)
			{
				PlannedCompletedInstances.Add(Inst);
			}
		}
	}
}

void UWS_Work::HandleInstanceCompleted(const FWorkInstance& Inst)
{
	const FWorkDefinitionRow* Def = FindDef(Inst.WorkId);
	if (!Def)
	{
		BroadcastRequestState(Inst.RequestId, EWorkRequestLifecycleState::Failed);
		return;
	}

	GrantWorkRewards(Def->Rewards);
	OnWorkCompleted.Broadcast(Inst.RequestId, Inst.WorkId);

	if (PopulationObj)
	{
		for (int32 PageId : Inst.Workers)
		{
			IEidosPopulationAccess::Execute_ClearPageWorkAssignment(PopulationObj, PageId, Inst.InstanceId);
			IEidosPopulationAccess::Execute_ApplyWorkCompletionEffects(PopulationObj, PageId, Inst.WorkId);
		}
	}

	BroadcastRequestState(Inst.RequestId, EWorkRequestLifecycleState::Completed);
}

const FWorkDefinitionRow* UWS_Work::FindDef(FName WorkId) const
{
	if (WorkId.IsNone())
	{
		return nullptr;
	}

	return WorkDefs.Find(WorkId);
}

bool UWS_Work::CancelWorkRequest(int32 RequestId)
{
	if (RequestId == INDEX_NONE)
	{
		return false;
	}

	const int32 RemovedQueued = Queue.RemoveAll([&](const FWorkRequest& Request)
	{
		return Request.RequestId == RequestId;
	});

	TArray<FWorkInstance> CancelledInstances;
	TSet<int32> CancelledInstanceIds;
	for (const FWorkInstance& Instance : ActiveInstances)
	{
		if (Instance.RequestId == RequestId)
		{
			CancelledInstances.Add(Instance);
			CancelledInstanceIds.Add(Instance.InstanceId);
		}
	}

	for (const FWorkInstance& Instance : PlannedNewInstances)
	{
		if (Instance.RequestId == RequestId)
		{
			CancelledInstanceIds.Add(Instance.InstanceId);
		}
	}

	const int32 RemovedActive = ActiveInstances.RemoveAll([&](const FWorkInstance& Instance)
	{
		return Instance.RequestId == RequestId;
	});
	const int32 RemovedPlanned = PlannedNewInstances.RemoveAll([&](const FWorkInstance& Instance)
	{
		return Instance.RequestId == RequestId;
	});
	PlannedCompletedInstances.RemoveAll([&](const FWorkInstance& Instance)
	{
		return Instance.RequestId == RequestId;
	});
	PlannedPageAssignments.RemoveAll([&](const FPlannedPageAssignment& Assignment)
	{
		return CancelledInstanceIds.Contains(Assignment.InstanceId);
	});

	if (RemovedQueued == 0 && RemovedActive == 0 && RemovedPlanned == 0)
	{
		return false;
	}

	// Costs are consumed when an instance starts. Aborting does not refund them,
	// but it immediately releases every Page that was working on this request.
	if (PopulationObj)
	{
		for (const FWorkInstance& Instance : CancelledInstances)
		{
			for (int32 PageId : Instance.Workers)
			{
				IEidosPopulationAccess::Execute_ClearPageWorkAssignment(PopulationObj, PageId, Instance.InstanceId);
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[Work] Cancelled request id=%d queued=%d active=%d planned=%d"), RequestId, RemovedQueued, RemovedActive, RemovedPlanned);
	BroadcastRequestState(RequestId, EWorkRequestLifecycleState::Cancelled);
	return true;
}

FVector UWS_Work::ResolveSiteLocationForWork(const FWorkDefinitionRow& Def) const
{
	const FVector SettlementOrigin = FVector::ZeroVector;

	if (Def.SiteTag.IsNone() || Def.SiteTag == FName("None"))
	{
		return SettlementOrigin;
	}
	if (Def.SiteTag == FName("Lumberyard"))
	{
		return SettlementOrigin + FVector(500.f, 0.f, 0.f);
	}
	if (Def.SiteTag == FName("Quarry"))
	{
		return SettlementOrigin + FVector(0.f, 500.f, 0.f);
	}
	if (Def.SiteTag == FName("Forge"))
	{
		return SettlementOrigin + FVector(-500.f, 0.f, 0.f);
	}
	if (Def.SiteTag == FName("Chapel"))
	{
		return SettlementOrigin + FVector(0.f, -500.f, 0.f);
	}

	return SettlementOrigin;
}

void UWS_Work::WriteToSnapshot_Implementation(FEidosWorldSnapshot& OutSnapshot) const
{
	OutSnapshot.Work.NextRequestId = NextRequestId;
	OutSnapshot.Work.NextInstanceId = NextInstanceId;
	OutSnapshot.Work.Queue = Queue;
	OutSnapshot.Work.ActiveInstances = ActiveInstances;
}

void UWS_Work::ApplySnapshot_Implementation(const FEidosWorldSnapshot& Snapshot)
{
	NextRequestId = FMath::Max(1, Snapshot.Work.NextRequestId);
	NextInstanceId = FMath::Max(1, Snapshot.Work.NextInstanceId);
	Queue = Snapshot.Work.Queue;
	ActiveInstances = Snapshot.Work.ActiveInstances;
	PlannedNewInstances.Reset();
	PlannedCompletedInstances.Reset();
	PlannedPageAssignments.Reset();

	for (const FWorkRequest& Request : Queue)
	{
		NextRequestId = FMath::Max(NextRequestId, Request.RequestId + 1);
	}
	for (const FWorkInstance& Instance : ActiveInstances)
	{
		NextInstanceId = FMath::Max(NextInstanceId, Instance.InstanceId + 1);
	}

	if (PopulationObj)
	{
		for (const FWorkInstance& Instance : ActiveInstances)
		{
			for (int32 PageId : Instance.Workers)
			{
				IEidosPopulationAccess::Execute_AssignPageToWork(
					PopulationObj,
					PageId,
					Instance.InstanceId,
					Instance.WorkId,
					Instance.SiteLocation,
					0);
			}
		}
	}
}

bool UWS_Work::HasQueuedRequest(int32 RequestId) const
{
	return Queue.ContainsByPredicate([&](const FWorkRequest& Request)
	{
		return Request.RequestId == RequestId;
	});
}

bool UWS_Work::HasActiveInstanceForRequest(int32 RequestId) const
{
	return ActiveInstances.ContainsByPredicate([&](const FWorkInstance& Instance)
	{
		return Instance.RequestId == RequestId;
	});
}

void UWS_Work::LoadWorkDefs()
{
	WorkDefs.Reset();

	if (!GetWorld())
	{
		return;
	}

	UGameInstance* GI = GetWorld()->GetGameInstance();
	if (!GI)
	{
		return;
	}

	UGIS_DataRegistry* Registry = GI->GetSubsystem<UGIS_DataRegistry>();
	if (!Registry)
	{
		return;
	}

	UDataTable* WorkTable = Registry->GetWorkTable();
	if (!WorkTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Work] DT_Work not found in DataRegistry"));
		return;
	}

	TArray<FWorkDefinitionRow*> Rows;
	WorkTable->GetAllRows<FWorkDefinitionRow>(TEXT("WS_Work::LoadWorkDefs"), Rows);

	for (const FWorkDefinitionRow* Row : Rows)
	{
		if (Row)
		{
			WorkDefs.Add(Row->WorkId, *Row);
		}
	}
}

void UWS_Work::BroadcastRequestState(int32 RequestId, EWorkRequestLifecycleState NewState)
{
	OnWorkRequestStateChanged.Broadcast(RequestId, NewState);
}
