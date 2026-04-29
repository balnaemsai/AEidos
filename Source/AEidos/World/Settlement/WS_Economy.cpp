// Fill out your copyright notice in the Description page of Project Settings.


#include "World/Settlement/WS_Economy.h"
#include "Data/GIS_DataRegistry.h"
#include "Data/Definitions/ResourceDefinitionRow.h"
#include "Engine/GameInstance.h"

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
	for (const FWorkCost& Cost : Costs)
	{
		if (Cost.ResourceId.IsNone())
		{
			UE_LOG(LogTemp, Warning, TEXT("[Economy] CanAfford: ResourceId is None"));
			return false;
		}

		if (Cost.Amount <= 0)
		{
			continue;
		}

		const int32 CurrentAmount = GetAmount(Cost.ResourceId);
		if (CurrentAmount < Cost.Amount)
		{
			UE_LOG(LogTemp, Log, TEXT("[Economy] CanAfford failed: Resource=%s Need=%d Have=%d"), *Cost.ResourceId.ToString(), Cost.Amount, CurrentAmount);
			return false;
		}
	}

	return true;
}

void UWS_Economy::ConsumeCosts_Implementation(const TArray<FWorkCost>& Costs)
{
	for (const FWorkCost& Cost : Costs)
	{
		if (Cost.ResourceId.IsNone())
		{
			UE_LOG(LogTemp, Warning, TEXT("[Economy] ConsumeCosts: ResourceId is None"));
			return;
		}

		if (Cost.Amount <= 0)
		{
			continue;
		}

		const int32 CurrentAmount = GetAmount(Cost.ResourceId);
		if (CurrentAmount < Cost.Amount)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[Economy] ConsumeCosts failed: Resource=%s Need=%d Have=%d"),
				*Cost.ResourceId.ToString(),
				Cost.Amount,
				CurrentAmount);
			return;
		}
	}

	for (const FWorkCost& Cost : Costs)
	{
		if (Cost.ResourceId.IsNone() || Cost.Amount <= 0)
		{
			continue;
		}

		AddAmount(Cost.ResourceId, -Cost.Amount);

		UE_LOG(LogTemp, Log, TEXT("[Economy] ConsumeCosts: Resource=%s Delta=-%d NewAmount=%d"), *Cost.ResourceId.ToString(), Cost.Amount, GetAmount(Cost.ResourceId));
	}
}

int32 UWS_Economy::GetResourceAmount_Implementation(FName ResourceId) const
{
	return GetAmount(ResourceId);
}

void UWS_Economy::GrantRewards_Implementation(const TArray<FWorkReward>& Rewards)
{
	for (const FWorkReward& Reward : Rewards)
	{
		if (Reward.ResourceId.IsNone())
		{
			UE_LOG(LogTemp, Warning, TEXT("[Economy] GrantRewards: ResourceId is None"));
			continue;
		}

		if (Reward.Amount == 0)
		{
			continue;
		}

		AddAmount(Reward.ResourceId, Reward.Amount);

		UE_LOG(LogTemp, Log,
			TEXT("[Economy] GrantRewards: Resource=%s Delta=+%d NewAmount=%d"),
			*Reward.ResourceId.ToString(),
			Reward.Amount,
			GetAmount(Reward.ResourceId));
	}
}

void UWS_Economy::ApplySnapshot_Implementation(const FEidosWorldSnapshot& Snapshot)
{
	Wallet.Amounts.Reset();

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Economy] ApplySnapshot failed: World is null"));
		bDirty = true;
		return;
	}

	UGameInstance* GI = World->GetGameInstance();
	if (!GI)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Economy] ApplySnapshot failed: GameInstance is null"));
		bDirty = true;
		return;
	}

	UGIS_DataRegistry* Registry = GI->GetSubsystem<UGIS_DataRegistry>();
	if (!Registry)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Economy] ApplySnapshot failed: DataRegistry is null"));
		bDirty = true;
		return;
	}

	const TArray<FName> ResourceIds = Registry->GetAllResourceIds();
	for (const FName ResourceId : ResourceIds)
	{
		const FResourceDefinitionRow* Def = Registry->GetResourceDef(ResourceId);
		if (!Def || !Def->bSavable)
		{
			continue;
		}

		const int32* SavedAmount = Snapshot.Economy.ResourceAmounts.Find(ResourceId);
		const int32 Amount = SavedAmount ? *SavedAmount : Def->DefaultStartingAmount;
		Wallet.Amounts.Add(ResourceId, Amount);
	}

	bDirty = true;
}

void UWS_Economy::WriteToSnapshot_Implementation(FEidosWorldSnapshot& InOutSnapshot) const
{
	InOutSnapshot.Economy.ResourceAmounts = Wallet.Amounts;
}





