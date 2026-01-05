// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WS_SimulationOrchestrator.generated.h"

class USimCommandBuffer;

DECLARE_LOG_CATEGORY_EXTERN(LogSimOrch, Log, All);

UENUM()
enum class ESimOrchState : uint8
{
	Stopped,
	Running,
	Paused
};

DECLARE_MULTICAST_DELEGATE(FEidosOnWorldSimReady);

/**
 * 
 */
UCLASS()
class AEIDOS_API UWS_SimulationOrchestrator : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UWS_SimulationOrchestrator, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return State == ESimOrchState::Running; }

	void StartMainLoop();
	void StopMainLoop();
	void SetPaused(bool bPaused);
	
	void RegisterSimSystem(UObject* System);
	
	FEidosOnWorldSimReady OnWorldSimReady;
	
	UPROPERTY(EditAnywhere, Category="Simulation")
	float FixedTickHz = 24.0f;

private:
	void BuildSystemCacheIfNeeded();
	void StepFixedTick(float FixedDeltaSeconds);

	UPROPERTY(Transient)
	ESimOrchState State = ESimOrchState::Stopped;

	UPROPERTY(Transient)
	TObjectPtr<USimCommandBuffer> CommandBuffer;

	// 고정틱 누적
	float Accumulator = 0.0f;

	// 등록된 시스템(원본)
	TArray<TWeakObjectPtr<UObject>> RegisteredSystems;

	// 실행용 캐시(정렬/유효성 반영된 리스트)
	TArray<TWeakObjectPtr<UObject>> CachedSystems;
	bool bCacheDirty = false;

	double SimStartRealTimeSec = 0.0;  
	double SimElapsedRealTimeSec = 0.0; 

	int64 FixedTickCount = 0;         
	double FixedSimulatedTimeSec = 0.0; 

	int32 DebugLogEveryNTicks = 24;
};
