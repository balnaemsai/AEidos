// Fill out your copyright notice in the Description page of Project Settings.


#include "World/Settlement/WS_Work.h"
#include "Simulation/SimCommandBuffer.h"
#include "World/Settlement/WS_Economy.h"
#include "World/Settlement/WS_Population.h"
#include "World/Settlement/EidosAccessInterface.h"
#include "Engine/World.h"
#include "Algo/Sort.h"
#include "Math/UnrealMathUtility.h"
#include "Data/GIS_DataRegistry.h"
#include "JsonObjectConverter.h"
#include "GameFramework/Actor.h"

void UWS_Work::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	EconomyObj = GetWorld()->GetSubsystem<UWS_Economy>();
	PopulationObj = GetWorld()->GetSubsystem<UWS_Population>();

	// 인터페이스 확인
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

	WorkDefs.Reset();

	if (!GetWorld()) return;

	UGameInstance* GI = GetWorld()->GetGameInstance();
	if (!GI) return;

	UGIS_DataRegistry* Registry = GI->GetSubsystem<UGIS_DataRegistry>();
	if (!Registry) return;

	UDataTable* WorkTable = Registry->GetWorkTable();
	if (!WorkTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Work] DT_Work not found in DataRegistry"));
		return;
	}

	TArray<FWorkDefinitionRow*> Rows;
	WorkTable->GetAllRows<FWorkDefinitionRow>(TEXT("WS_Work::Initialize"), Rows);

	for (const FWorkDefinitionRow* Row : Rows)
	{
		if (!Row) continue;
		WorkDefs.Add(Row->WorkId, *Row);
	}

	UE_LOG(LogTemp, Log, TEXT("[Work] Loaded WorkDefs: %d"), WorkDefs.Num());
}

void UWS_Work::SimPlan_Implementation(USimCommandBuffer* CommandBuffer, float FixedDeltaSeconds)
{
	// 1초당 생산량을 FixedDelta에 따라 분배
	//ProductionAccumulator += Producer.OutputPerGameSecond;

	PlannedNewInstances.Reset();
	PlannedCompletedInstances.Reset();

	// 1) 새 작업 인스턴스 생성 후보 수집
	TrySpawnInstancesFromQueue();

	// 2) 배정 갱신
	UpdateAssignments(FixedDeltaSeconds);

	// 3) 진행도 누적 + 완료 목록 수집
	ProgressInstances(FixedDeltaSeconds);

	/*
	
	const int32 ProduceNow = FMath::FloorToInt(ProductionAccumulator);
	if (ProduceNow > 0)
	{
		ProductionAccumulator -= (float)ProduceNow;
		PlannedDeltaInt += ProduceNow;
	}
	UE_LOG(LogTemp, Log, TEXT("[Work] Plan: Acc=%.3f"), ProductionAccumulator);
	*/
}

void UWS_Work::SimCommit_Implementation(USimCommandBuffer* CommandBuffer, float FixedDeltaSeconds)
{
	if (!CommandBuffer)
		return;

	TWeakObjectPtr<UObject> WeakEco(EconomyObj);
	TWeakObjectPtr<UWS_Work> WeakThis(this);

	UWorld* World = GetWorld();
	if (!World) return;

	UWS_Economy* Economy = World->GetSubsystem<UWS_Economy>();
	if (!Economy) return;

	for (const FWorkInstance& Planned : PlannedNewInstances)
	{
		const FWorkDefinitionRow* Def = FindDef(Planned.WorkId);
		if (!Def) continue;

		FWorkInstance Inst = Planned;
		Inst.InstanceId = NextInstanceId++;

		TArray<FWorkCost> Costs = Def->Costs;

		CommandBuffer->Enqueue([WeakThis, WeakEco, Inst, Costs]()
		{
			if (UWS_Work* Work = WeakThis.Get())
			{
				Work->ActiveInstances.Add(Inst);
			}

			if (UObject* EcoObj = WeakEco.Get())
			{
				IEidosEconomyAccess::Execute_ConsumeCosts(EcoObj, Costs);
			}
		});
	}

	for (const FWorkInstance& Completed : PlannedCompletedInstances)
	{
		CommandBuffer->Enqueue([WeakThis, Completed]()
		{
			if (UWS_Work* Work = WeakThis.Get())
			{
				Work->ActiveInstances.RemoveAll([&](const FWorkInstance& I)
				{
					return I.InstanceId == Completed.InstanceId;
				});

				Work->HandleInstanceCompleted(Completed);
			}
		});
	}

	Queue.RemoveAll([](const FWorkRequest& R)
	{
		return R.Mode == EWorkRequestMode::Count && R.RemainingCount <= 0;
	});
}

int32 UWS_Work::AddWorkRequest(const FWorkRequest& InReq)
{
	FWorkRequest Req = InReq;
	Req.RequestId = NextRequestId++;

	if (Req.Mode == EWorkRequestMode::Count && Req.RemainingCount <= 0)
		Req.RemainingCount = 1;

	Queue.Add(Req);

	// Priority 높은 것 먼저 처리하고 싶으면 정렬
	Queue.Sort([](const FWorkRequest& A, const FWorkRequest& B)
	{
		if (A.Priority != B.Priority) return A.Priority > B.Priority;
		return A.RequestId < B.RequestId;
	});

	UE_LOG(LogTemp, Log, TEXT("[Work] AddWorkRequest id=%d work=%s mode=%d prio=%d"),
		Req.RequestId, *Req.WorkId.ToString(), (int32)Req.Mode, Req.Priority);

	return Req.RequestId;
}

bool UWS_Work::IsRequestSatisfied(const FWorkRequest& Req) const
{
	if (!EconomyObj) return false;

	if (Req.Mode == EWorkRequestMode::Until)
	{
		const int32 Cur = IEidosEconomyAccess::Execute_GetResourceAmount(EconomyObj, Req.UntilResourceId);
		return Cur >= Req.UntilTargetAmount;
	}
	return false;
}

void UWS_Work::TrySpawnInstancesFromQueue()
{
	if (!EconomyObj) return;

	for (int32 i = 0; i < Queue.Num(); ++i)
	{
		FWorkRequest& Req = Queue[i];

		if (Req.Mode == EWorkRequestMode::Count && Req.RemainingCount <= 0)
		{
			continue;
		}
		if (Req.Mode == EWorkRequestMode::Until && IsRequestSatisfied(Req))
		{
			continue;
		}

		const FWorkDefinitionRow* Def = FindDef(Req.WorkId);
		if (!Def) continue;

		// 입력 자원 체크 (전략: “인스턴스 시작 시 선소모” MVP)
		if (!IEidosEconomyAccess::Execute_CanAfford(EconomyObj, Def->Costs))
		{
			// 자원 부족이면 일단 스킵 (나중에 자원이 생기면 다시 시도)
			continue;
		}

		// 인스턴스 생성 (동시에 너무 많이 생기는 걸 막고 싶으면 여기서 제한)
		FWorkInstance Inst;
		Inst.InstanceId = NextInstanceId++;
		Inst.RequestId = Req.RequestId;
		Inst.WorkId = Req.WorkId;
		Inst.TotalWork = Def->TotalWork;
		Inst.Progress = 0.f;
		Inst.MaxWorkers = Def->MaxWorkers;
		Inst.SiteLocation = ResolveSiteLocationForWork(*Def);
		
		PlannedNewInstances.Add(Inst);

		// Count 모드면 바로 1회 차감(“작업 시작” 기준)
		if (Req.Mode == EWorkRequestMode::Count)
		{
			Req.RemainingCount -= 1;
		}

		// Repeat/Until은 계속 생성될 수 있으니, 과도 생성 방지하려면 break/limit 추천
		// MVP: 한 틱에 큐당 1개만 생성
		break;
	}
}

void UWS_Work::UpdateAssignments(float FixedDeltaSeconds)
{
	if (!PopulationObj) return;

	TArray<int32> Pages = IEidosPopulationAccess::Execute_GetAllPageIds(PopulationObj);

	for (FWorkInstance& Inst : ActiveInstances)
	{
		const FWorkDefinitionRow* Def = FindDef(Inst.WorkId);
		if (!Def) continue;

		// 부족한 인원만큼 채우기
		while (Inst.Workers.Num() < Inst.MaxWorkers)
		{
			int32 BestPageId = INDEX_NONE;

			for (int32 PageId : Pages)
			{
				if (!IEidosPopulationAccess::Execute_IsPageAvailable(PopulationObj, PageId))
					continue;

				// 이미 이 인스턴스에 참여 중이면 스킵
				if (Inst.Workers.Contains(PageId))
					continue;

				// TODO: 페이지의 job 우선순위/선호/해금(특성) 체크 넣고 싶으면 여기서 판단
				BestPageId = PageId;
				break;
			}

			if (BestPageId == INDEX_NONE)
				break;

			Inst.Workers.Add(BestPageId);

			// PageJobs에 기록 (페이지가 여러 job을 가질 수 있으니 배열)
			FJob Job;
			Job.PageId = BestPageId;
			Job.InstanceId = Inst.InstanceId;
			Job.Priority = 0;
			Job.bIsActive = true;

			PageJobs.FindOrAdd(BestPageId).Jobs.Add(Job);
		}
		
	}
}

void UWS_Work::ProgressInstances(float FixedDeltaSeconds)
{
	if (!PopulationObj) return;

	for (FWorkInstance& Inst : ActiveInstances)
	{
		const FWorkDefinitionRow* Def = FindDef(Inst.WorkId);
		if (!Def) continue;

		float TotalRateThisTick = 0.f;

		for (int32 PageId : Inst.Workers)
		{
			const float Mult = IEidosPopulationAccess::Execute_ComputeWorkRateMultiplier(PopulationObj, PageId, Inst.WorkId);
			const float Rate = Def->BaseWorkRate * Mult;

			TotalRateThisTick += Rate;
		}

		Inst.Progress += TotalRateThisTick * FixedDeltaSeconds;

		if (Inst.Progress >= Inst.TotalWork)
		{
			if (!PlannedCompletedInstances.ContainsByPredicate([&](const FWorkInstance& I)
			{
				return I.InstanceId == Inst.InstanceId;
			}))
			{
				PlannedCompletedInstances.Add(Inst);
			}
		}
	}
}

void UWS_Work::HandleInstanceCompleted(const FWorkInstance& Inst)
{
	const FWorkDefinitionRow* Def = FindDef(Inst.WorkId);
	if (!Def) return;

	// 보상 지급
	if (EconomyObj)
	{
		IEidosEconomyAccess::Execute_GrantRewards(EconomyObj, Def->Rewards);
	}

	// 페이지 성장
	if (PopulationObj)
	{
		for (int32 PageId : Inst.Workers)
		{
			IEidosPopulationAccess::Execute_ApplyWorkCompletionEffects(PopulationObj, PageId, Inst.WorkId);
		}
	}

	// Job 정리
	for (int32 PageId : Inst.Workers)
	{
		if (FJobArray* Arr = PageJobs.Find(PageId))
		{
			Arr->Jobs.RemoveAll([&](const FJob& J)
			{
				return J.InstanceId == Inst.InstanceId;
			});
		}
	}

	// Request 모드에 따른 후속은 “TrySpawnInstancesFromQueue”가 다음 틱에서 알아서 뽑도록 두면 단순해짐
}

const FWorkDefinitionRow* UWS_Work::FindDef(FName WorkId) const
{
	if (WorkId.IsNone())
	{
		return nullptr;
	}

	if (const FWorkDefinitionRow* Found = WorkDefs.Find(WorkId))
	{
		return Found;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Work] FindDef failed. WorkId=%s"), *WorkId.ToString());
	return nullptr;
}

bool UWS_Work::CancelWorkRequest(int32 RequestId)
{
	// 1) 큐에서 제거 시도
	const int32 Removed = Queue.RemoveAll([&](const FWorkRequest& R)
	{
		return R.RequestId == RequestId;
	});

	if (Removed > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[Work] CancelWorkRequest id=%d (removed from queue)"), RequestId);
		return true;
	}

	// 2) 실행 중이면 MVP 정책상 취소 불가
	const bool bHasActive = ActiveInstances.ContainsByPredicate([&](const FWorkInstance& I)
	{
		return I.RequestId == RequestId;
	});

	if (bHasActive)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Work] CancelWorkRequest id=%d denied: active instance exists"), RequestId);
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Work] CancelWorkRequest id=%d failed: not found"), RequestId);
	return false;
}

FVector UWS_Work::ResolveSiteLocationForWork(const FWorkDefinitionRow& Def) const
{
	// MVP: SiteTag에 따라 임의 오프셋을 주어 “서로 다른 작업장 느낌”만 낸다
	const FVector SettlementOrigin = FVector::ZeroVector;

	if (Def.SiteTag.IsNone() || Def.SiteTag == FName("None"))
	{
		return SettlementOrigin;
	}

	// 간단한 매핑 예시 (원하는대로 바꿔)
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

	// 미등록 태그면 원점
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
	NextRequestId = Snapshot.Work.NextRequestId;
	NextInstanceId = Snapshot.Work.NextInstanceId;

	Queue = Snapshot.Work.Queue;
	ActiveInstances = Snapshot.Work.ActiveInstances;

	// 런타임 캐시 초기화(다음 틱에 재할당)
	PageJobs.Reset();

	// 방어적: ID가 0/미설정으로 들어왔을 때 대비
	if (NextRequestId <= 0) NextRequestId = 1;
	if (NextInstanceId <= 0) NextInstanceId = 1;

	// 방어적: 로드된 데이터 기반으로 NextId 재계산(충돌 방지)
	for (const FWorkRequest& R : Queue)
		NextRequestId = FMath::Max(NextRequestId, R.RequestId + 1);

	for (const FWorkInstance& I : ActiveInstances)
		NextInstanceId = FMath::Max(NextInstanceId, I.InstanceId + 1);
}




