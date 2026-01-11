// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Simulation/SimSystem.h"
#include "WS_Economy.generated.h"

class USimCommandBuffer;

/**
 * 
 */

USTRUCT(BlueprintType)
struct FResourceWallet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<FName, int32> Amounts;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEconomyChanged);

UCLASS()
class AEIDOS_API UWS_Economy : public UWorldSubsystem, public ISimSystem
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnEconomyChanged OnEconomyChanged;

	int32 GetAmount(FName ResourceId) const;
	void AddAmount(FName ResourceId, int32 Delta);

	virtual void SimPlan_Implementation(USimCommandBuffer* CommandBuffer, float FixedDeltaSeconds) override {}
	virtual void SimCommit_Implementation(USimCommandBuffer* CommandBuffer, float FixedDeltaSeconds) override {}
	virtual void SimPost_Implementation(float FixedDeltaSeconds) override;
	virtual int32 GetSimOrder_Implementation() const override { return 100; }

private:
	UPROPERTY()
	FResourceWallet Wallet;

	bool bDirty = false;
};
