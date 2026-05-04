// Fill out your copyright notice in the Description page of Project Settings.


#include "World/Settlement/WS_Work.h"

#include "Data/GIS_DataRegistry.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Simulation/SimCommandBuffer.h"
#include "World/Settlement/EidosAccessInterface.h"
#include "World/Settlement/WS_Economy.h"
#include "World/Settlement/WS_Population.h"

void UWS_Work::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Collection.InitializeDependency<UWS_Economy>();
	Collection.InitializeDependency<UWS_Population>();
	EconomyObj = GetWorld()->GetSubsystem<UWS_Economy>();
	PopulationObj = GetWorld()->GetSubsystem<UWS_Population>();

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

	TWeakObjectPtr<UObject> WeakEco(EconomyObj);
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

		CommandBuffer->Enqueue([WeakThis, WeakEco, Inst, Costs]()
		{
			if (UWS_Work* Work = WeakThis.Get())
			{
				Work->ActiveInstances.Add(Inst);
				Work->BroadcastRequestState(Inst.RequestId, EWorkRequestLifecycleState::Active);
			}

			if (UObject* EcoObj = WeakEco.Get())
			{
				IEidosEconomyAccess::Execute_ConsumeCosts(EcoObj, Costs);
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

	Queue.RemoveAll([](const FWorkRequest& Request)
	{
		return Request.Mode == EWorkRequestMode::Count && Request.RemainingCount <= 0;
	});
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

		if (!IEidosEconomyAccess::Execute_CanAfford(EconomyObj, Def->Costs))
		{
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
		if (Inst.Progress >= Inst.TotalWork)
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

	if (EconomyObj)
	{
		IEidosEconomyAccess::Execute_GrantRewards(EconomyObj, Def->Rewards);
	}

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
	const int32 Removed = Queue.RemoveAll([&](const FWorkRequest& Request)
	{
		return Request.RequestId == RequestId;
	});

	if (Removed > 0)
	{
		BroadcastRequestState(RequestId, EWorkRequestLifecycleState::Cancelled);
		return true;
	}

	const bool bHasActive = ActiveInstances.ContainsByPredicate([&](const FWorkInstance& Instance)
	{
		return Instance.RequestId == RequestId;
	});

	if (bHasActive)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Work] CancelWorkRequest id=%d denied: active instance exists"), RequestId);
		return false;
	}

	return false;
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
