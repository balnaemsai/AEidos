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
	int32& V = Wallet.Amounts.FindOrAdd(ResourceId);
	V += Delta;
	bDirty = true;
}

void UWS_Economy::SimPost_Implementation(float FixedDeltaSeconds)
{
	if (bDirty)
	{
		bDirty = false;
		OnEconomyChanged.Broadcast();
	}
}