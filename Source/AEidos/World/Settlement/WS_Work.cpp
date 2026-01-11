// Fill out your copyright notice in the Description page of Project Settings.


#include "World/Settlement/WS_Work.h"
#include "Simulation/SimCommandBuffer.h"
#include "World/Settlement/WS_Economy.h"
#include "Engine/World.h"

void UWS_Work::SimPlan_Implementation(USimCommandBuffer* CommandBuffer, float FixedDeltaSeconds)
{
	// 1초당 생산량을 FixedDelta에 따라 분배
	ProductionAccumulator += Producer.OutputPerGameSecond * FixedDeltaSeconds;

	const int32 ProduceNow = FMath::FloorToInt(ProductionAccumulator);
	if (ProduceNow > 0)
	{
		ProductionAccumulator -= (float)ProduceNow;
		PlannedDeltaInt += ProduceNow;
	}
}

void UWS_Work::SimCommit_Implementation(USimCommandBuffer* CommandBuffer, float FixedDeltaSeconds)
{
	if (!CommandBuffer || PlannedDeltaInt <= 0)
		return;

	UWorld* World = GetWorld();
	if (!World) return;

	UWS_Economy* Economy = World->GetSubsystem<UWS_Economy>();
	if (!Economy) return;

	const FName ResId = Producer.OutputResourceId;
	const int32 Delta = PlannedDeltaInt;

	// ✅ 월드 상태 변경은 Flush에서만 일어나게 커맨드로 예약
	TWeakObjectPtr<UWS_Economy> WeakEco(Economy);
	CommandBuffer->Enqueue([WeakEco, ResId, Delta]()
	{
		if (UWS_Economy* Eco = WeakEco.Get())
		{
			Eco->AddAmount(ResId, Delta);
		}
	});

	PlannedDeltaInt = 0;
}

