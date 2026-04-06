// Fill out your copyright notice in the Description page of Project Settings.


#include "World/Settlement/WS_Economy.h"

int32 UWS_Economy::GetAmount(FName ResourceId) const
{
	if (const int32* Found = Wallet.Amounts.Find(ResourceId))
	{
		return *Found;
	}
	return 0;
}

void UWS_Economy::AddAmount(FName ResourceId, int32 Delta)
{
	if (ResourceId.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Economy] AddAmount_Internal called with None ResourceId"));
		return;
	}
	
	int32& V = Wallet.Amounts.FindOrAdd(ResourceId);
	V += Delta;
	bDirty = true;

	UE_LOG(LogTemp, Log, TEXT("[Economy] %s += %d (Total=%d)"), *ResourceId.ToString(), Delta, V);
}

void UWS_Economy::SimPost_Implementation(float FixedDeltaSeconds)
{
	UE_LOG(LogTemp, Log, TEXT("[Economy] Post: Broadcast OnEconomyChanged"));
	if (bDirty)
	{
		bDirty = false;
		OnEconomyChanged.Broadcast();
	}
}

bool UWS_Economy::CanAfford_Implementation(const TArray<FWorkCost>& Costs) const
{
	return true;
}

void UWS_Economy::ConsumeCosts_Implementation(const TArray<FWorkCost>& Costs)
{
	
}

int32 UWS_Economy::GetResourceAmount_Implementation(FName ResourceId) const
{
	return GetAmount(ResourceId);
}

void UWS_Economy::GrantRewards_Implementation(const TArray<FWorkReward>& Rewards)
{
	
}



