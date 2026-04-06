// Fill out your copyright notice in the Description page of Project Settings.


#include "World/Settlement/WS_Population.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Simulation/WS_SimulationOrchestrator.h"
#include "Simulation/SimCommandBuffer.h"
#include "EngineUtils.h"

#include "Entities/Page/PageCharacter.h"
#include "Entities/Page/Components/StatsComponent.h"
#include "Entities/Page/Components/SkillComponent.h"

void UWS_Population::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	bCacheDirty = true;

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
			UE_LOG(LogTemp, Warning, TEXT("[SettlementSpace] Failed to load ChunkActor BP class: %s. Fallback to native."), PageBPClassPath);
			TestPageClass = APageCharacter::StaticClass();
		}
	}
}

void UWS_Population::Deinitialize()
{
	CachedPages.Reset();
	PlannedDeltas.Reset();
	Super::Deinitialize();
}

int32 UWS_Population::GetSimOrder_Implementation() const
{
	// Work/Economy와 순서 조정 가능
	// 예: Population(10) -> Work(20) -> Economy(30)
	return 10;
}

void UWS_Population::RebuildCacheIfNeeded()
{
	if (!bCacheDirty)
	{
		return;
	}

	CachedPages.Reset();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(World, APageCharacter::StaticClass(), Found);

	for (AActor* A : Found)
	{
		if (APageCharacter* P = Cast<APageCharacter>(A))
		{
			CachedPages.Add(P);
		}
	}

	PlannedDeltas.SetNum(CachedPages.Num());
	for (FPageStatsDelta& D : PlannedDeltas)
	{
		D = FPageStatsDelta{};
	}

	bCacheDirty = false;
}

void UWS_Population::SimPlan_Implementation(USimCommandBuffer* CommandBuffer, float FixedDeltaSeconds)
{
	RebuildCacheIfNeeded();

	// 예시 규칙: 게임시간 1초(=FixedTick 1회)마다 허기/피로 증가
	// (네 Orchestrator에서 FixedTickHz=24면, 1틱=게임 1초라는 네 정의에 맞춤)
	for (int32 i = 0; i < CachedPages.Num(); ++i)
	{
		APageCharacter* Page = CachedPages[i].Get();
		if (!Page)
		{
			continue;
		}

		// 여기서 FixedDeltaSeconds는 “게임 1초”로 쓰겠다는 네 정의면 굳이 곱하지 않아도 됨.
		// 일단 규칙값을 작게 잡아 예시로:
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

	// 변경은 Flush에서만 일어나도록 커맨드로 예약
	for (int32 i = 0; i < CachedPages.Num(); ++i)
	{
		TWeakObjectPtr<APageCharacter> WeakPage = CachedPages[i];
		const FPageStatsDelta Delta = PlannedDeltas[i];

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
	// 필요 시: 변경된 페이지 목록 브로드캐스트 / 통계 / 로그 등
}

void UWS_Population::EnsureTestPageSpawned()
{
	if (!bSpawnTestPage) return;

	UWorld* World = GetWorld();
	if (!World) return;
	
	bool bAnyPageExists = false;
	for (TActorIterator<APageCharacter> It(World); It; ++It)
	{
		bAnyPageExists = true;
		break;
	}

	if (bAnyPageExists)
		return;

	TSubclassOf<APageCharacter> SpawnClass = TestPageClass;
	if (!SpawnClass)
	{
		SpawnClass = APageCharacter::StaticClass();
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	World->SpawnActor<APageCharacter>(SpawnClass, TestSpawnLocation, FRotator::ZeroRotator, Params);
	
	bCacheDirty = true;
}

TArray<int32> UWS_Population::GetAllPageIds_Implementation() const
{
	UWS_Population* MutableThis = const_cast<UWS_Population*>(this);
	MutableThis->RebuildCacheIfNeeded();

	TArray<int32> Result;
	Result.Reserve(MutableThis->CachedPages.Num());

	for (int32 i = 0; i < MutableThis->CachedPages.Num(); ++i)
	{
		if (MutableThis->CachedPages[i].IsValid())
		{
			Result.Add(i);
		}
	}

	return Result;
}

AActor* UWS_Population::GetPageActor_Implementation(int32 PageId)
{
	RebuildCacheIfNeeded();

	if (!CachedPages.IsValidIndex(PageId))
	{
		return nullptr;
	}

	return CachedPages[PageId].Get();
}

bool UWS_Population::IsPageAvailable_Implementation(int32 PageId) const
{
	UWS_Population* MutableThis = const_cast<UWS_Population*>(this);
	MutableThis->RebuildCacheIfNeeded();

	if (!MutableThis->CachedPages.IsValidIndex(PageId))
	{
		return false;
	}

	APageCharacter* Page = MutableThis->CachedPages[PageId].Get();
	if (!Page)
	{
		return false;
	}

	// TODO:
	// 나중엔 현재 job 상태, down 상태, 수면 상태 등을 반영
	return true;
}

float UWS_Population::ComputeWorkRateMultiplier_Implementation(int32 PageId, FName WorkId) const
{
	UWS_Population* MutableThis = const_cast<UWS_Population*>(this);
	MutableThis->RebuildCacheIfNeeded();

	if (!MutableThis->CachedPages.IsValidIndex(PageId))
	{
		return 1.f;
	}

	APageCharacter* Page = MutableThis->CachedPages[PageId].Get();
	if (!Page)
	{
		return 1.f;
	}

	// 가장 단순한 현재 구조:
	// WorkId == SkillId 라고 가정하거나,
	// 나중에 DT_Work에서 PrimarySkillId를 꺼내 Population 쪽에서 사용하도록 확장 가능

	//UE_LOG(LogTemp, Log, TEXT("[Population] ComputeWorkRate: Skillmultiplier is %f"), Page->GetSkillMultiplier(WorkId));
	return Page->GetSkillMultiplier(WorkId);
}

void UWS_Population::ApplyWorkCompletionEffects_Implementation(int32 PageId, FName WorkId)
{
	RebuildCacheIfNeeded();

	if (!CachedPages.IsValidIndex(PageId))
	{
		return;
	}

	APageCharacter* Page = CachedPages[PageId].Get();
	if (!Page)
	{
		return;
	}

	// 완료 보너스 XP 같은 게 필요하면 여기서 지급
	// 현재는 WS_Work의 진행 중 XP 지급 구조가 있으므로 비워둬도 됨
}