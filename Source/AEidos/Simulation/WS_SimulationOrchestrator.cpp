// Fill out your copyright notice in the Description page of Project Settings.


#include "Simulation/WS_SimulationOrchestrator.h"
#include "Simulation/SimCommandBuffer.h"
#include "Simulation/SimSystem.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY(LogSimOrch);

void UWS_SimulationOrchestrator::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	CommandBuffer = NewObject<USimCommandBuffer>(this);
	Accumulator = 0.0f;
	State = ESimOrchState::Stopped;

	RegisteredSystems.Reset();
	CachedSystems.Reset();
	bCacheDirty = false;

	UE_LOG(LogSimOrch, Log, TEXT("[SimOrch] Initialize. World=%s"), *GetWorld()->GetMapName());
}

void UWS_SimulationOrchestrator::Deinitialize()
{
	UE_LOG(LogSimOrch, Log, TEXT("[SimOrch] Deinitialize."));

	RegisteredSystems.Reset();
	CachedSystems.Reset();
	CommandBuffer = nullptr;

	Super::Deinitialize();
}

void UWS_SimulationOrchestrator::StartMainLoop()
{
	if (State == ESimOrchState::Running)
		return;

	State = ESimOrchState::Running;
	Accumulator = 0.0f;

	SimStartRealTimeSec = FPlatformTime::Seconds();
	SimElapsedRealTimeSec = 0.0;
	FixedTickCount = 0;
	FixedSimulatedTimeSec = 0.0;

	BuildSystemCacheIfNeeded();

	UE_LOG(LogSimOrch, Log, TEXT("[SimOrch] StartMainLoop. Systems=%d"), CachedSystems.Num());

	UE_LOG(LogSimOrch, Log, TEXT("[SimOrch] StartMainLoop. Map=%s Systems=%d FixedHz=%.2f"),
		GetWorld() ? *GetWorld()->GetMapName() : TEXT("null"),
		CachedSystems.Num(),
		FixedTickHz);
	
	OnWorldSimReady.Broadcast();
}

void UWS_SimulationOrchestrator::StopMainLoop()
{
	State = ESimOrchState::Stopped;
	Accumulator = 0.0f;
	if (CommandBuffer) CommandBuffer->Reset();

	UE_LOG(LogSimOrch, Log, TEXT("[SimOrch] StopMainLoop."));
}

void UWS_SimulationOrchestrator::SetPaused(bool bPaused)
{
	if (State == ESimOrchState::Stopped)
		return;

	State = bPaused ? ESimOrchState::Paused : ESimOrchState::Running;
	UE_LOG(LogSimOrch, Log, TEXT("[SimOrch] SetPaused=%s"), bPaused ? TEXT("true") : TEXT("false"));
}

void UWS_SimulationOrchestrator::RegisterSimSystem(UObject* System)
{
	if (!System)
		return;

	RegisteredSystems.Add(System);
	bCacheDirty = true;

	UE_LOG(LogSimOrch, Log, TEXT("[SimOrch] RegisterSimSystem: %s"), *System->GetName());
}

void UWS_SimulationOrchestrator::BuildSystemCacheIfNeeded()
{
	if (!bCacheDirty && CachedSystems.Num() > 0)
		return;

	// 유효한 시스템만 모으고
	CachedSystems.Reset();
	for (TWeakObjectPtr<UObject>& W : RegisteredSystems)
	{
		if (UObject* Obj = W.Get())
		{
			// SimSystem 구현한 것만 실행대상
			if (Obj->GetClass()->ImplementsInterface(USimSystem::StaticClass()))
			{
				CachedSystems.Add(Obj);
			}
		}
	}

	// Order로 정렬
	CachedSystems.Sort([](const TWeakObjectPtr<UObject>& A, const TWeakObjectPtr<UObject>& B)
	{
		UObject* OA = A.Get();
		UObject* OB = B.Get();
		if (!OA || !OB) return false;

		const int32 OrderA = ISimSystem::Execute_GetSimOrder(OA);
		const int32 OrderB = ISimSystem::Execute_GetSimOrder(OB);
		if (OrderA != OrderB) return OrderA < OrderB;

		// 동일 오더면 이름으로 안정 정렬
		return OA->GetName() < OB->GetName();
	});

	bCacheDirty = false;
}

//Tick마다 현실 시간(DeltaTime)을 Accumulator에 더하고, 이것이 정해준 FixedTick(1/24초)를 넘을 경우 게임 시간 1초가 지났다고 판정하며 StepFixedTick()을 한번 호출
//게임 틱이 느려져서 Tick 한번당 게임 시간 2초 이상이 흘러도 while문 내에서 여러번 호출해주기때문에 ㄱㅊ
void UWS_SimulationOrchestrator::Tick(float DeltaTime)
{
	if (State != ESimOrchState::Running)
		return;

	if (FixedTickHz <= 0.0f)
		return;

	SimElapsedRealTimeSec = FPlatformTime::Seconds() - SimStartRealTimeSec;

	BuildSystemCacheIfNeeded();

	const float FixedDelta = 1.0f / FixedTickHz;
	Accumulator += DeltaTime;
	
	int32 MaxStepsThisFrame = 8;
	while (Accumulator >= FixedDelta && MaxStepsThisFrame-- > 0)
	{
		Accumulator -= FixedDelta;
		StepFixedTick(FixedDelta);
	}
}

void UWS_SimulationOrchestrator::StepFixedTick(float FixedDeltaSeconds)
{
	if (!CommandBuffer)
		return;

	// -------- PLAN --------
	// 월드 직접 변경 금지(권장). 변경은 Cmd에 Enqueue.
	for (TWeakObjectPtr<UObject>& W : CachedSystems)
	{
		if (UObject* Sys = W.Get())
		{
			ISimSystem::Execute_SimPlan(Sys, CommandBuffer, FixedDeltaSeconds);
		}
	}

	for (TWeakObjectPtr<UObject>& W : CachedSystems)
	{
		if (UObject* Sys = W.Get())
		{
			ISimSystem::Execute_SimCommit(Sys, CommandBuffer, FixedDeltaSeconds);
		}
	}
	
	CommandBuffer->Flush();

	// -------- POST --------
	for (TWeakObjectPtr<UObject>& W : CachedSystems)
	{
		if (UObject* Sys = W.Get())
		{
			ISimSystem::Execute_SimPost(Sys, FixedDeltaSeconds);
		}
	}

	FixedTickCount++;
	FixedSimulatedTimeSec += FixedDeltaSeconds;

	// N틱마다 출력 (기본: 24틱=1초마다 한 번)
	
	if (DebugLogEveryNTicks > 0 && (FixedTickCount % DebugLogEveryNTicks) == 0)
	{
		UE_LOG(LogSimOrch, Log,
			TEXT("[SimOrch] FixedTick #%lld | RealElapsed=%.3fs | Simulated=%.3fs | Acc=%.4f | CachedSystems=%d"),
			FixedTickCount,
			SimElapsedRealTimeSec,
			FixedSimulatedTimeSec,
			Accumulator,
			CachedSystems.Num());
	}
	
}



