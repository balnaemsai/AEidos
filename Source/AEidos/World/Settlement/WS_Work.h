// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Simulation/SimSystem.h"
#include "WS_Work.generated.h"

class USimCommandBuffer;

/**
 * 
 */

USTRUCT()
struct FWorkProducer
{
	GENERATED_BODY()

	UPROPERTY()
	FName OutputResourceId = "Food";

	/** 게임 시간 1초당 생산량 */
	UPROPERTY()
	float OutputPerGameSecond = 1.0f;
};

UCLASS()
class AEIDOS_API UWS_Work : public UWorldSubsystem, public ISimSystem
{
	GENERATED_BODY()

public:
	virtual void SimPlan_Implementation(USimCommandBuffer* Cmd, float FixedDeltaSeconds) override;
	virtual void SimCommit_Implementation(USimCommandBuffer* Cmd, float FixedDeltaSeconds) override;
	virtual void SimPost_Implementation(float FixedDeltaSeconds) override {}
	virtual int32 GetSimOrder_Implementation() const override { return 50; }

private:
	UPROPERTY()
	FWorkProducer Producer;

	float ProductionAccumulator = 0.0f;

	int32 PlannedDeltaInt = 0;
	
};
