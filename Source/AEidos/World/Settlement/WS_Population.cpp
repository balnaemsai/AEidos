// Fill out your copyright notice in the Description page of Project Settings.


#include "World/Settlement/WS_Population.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Simulation/WS_SimulationOrchestrator.h"
#include "Simulation/SimCommandBuffer.h"
#include "EngineUtils.h"

#include "Entities/Page/PageCharacter.h"
#include "Entities/Page/Components/StatsComponent.h"

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
