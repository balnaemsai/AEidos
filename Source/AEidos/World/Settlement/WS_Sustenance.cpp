// Fill out your copyright notice in the Description page of Project Settings.

#include "World/Settlement/WS_Sustenance.h"

#include "Entities/Page/PageCharacter.h"
#include "Data/Definitions/ItemDefinitionRow.h"
#include "Data/GIS_DataRegistry.h"
#include "Engine/GameInstance.h"
#include "Entities/Items/InventoryComponent.h"
#include "World/Settlement/WS_ItemStorage.h"
#include "World/Settlement/WS_Population.h"

void UWS_Sustenance::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	StoredMealUnits = 0.f;
	StoredMealQualityTotal = 0.f;
	LastServedAverageMealQuality = 0.f;
	CurrentDailyMealDemandUnits = 0.f;
	LastKnownPopulation = 0;
	LastMealCoverage = 1.f;
	bFoodShortage = false;
	MealServiceElapsedSeconds = 0.f;
	bDirty = false;
}

void UWS_Sustenance::SimPlan_Implementation(USimCommandBuffer* CommandBuffer, float FixedDeltaSeconds)
{
	RefreshDemandFromPopulation();
	RefreshMealStorageFromWarehouse();

	MealServiceElapsedSeconds += FMath::Max(0.f, FixedDeltaSeconds);
	while (MealServiceElapsedSeconds >= MealServiceIntervalSeconds)
	{
		MealServiceElapsedSeconds -= MealServiceIntervalSeconds;
		ServeSettlementMeal();
	}
}

void UWS_Sustenance::SimPost_Implementation(float FixedDeltaSeconds)
{
	if (bDirty)
	{
		bDirty = false;
		OnSustenanceChanged.Broadcast();
	}
}

void UWS_Sustenance::WriteToSnapshot_Implementation(FEidosWorldSnapshot& InOutSnapshot) const
{
	InOutSnapshot.Sustenance.StoredMealUnits = StoredMealUnits;
	InOutSnapshot.Sustenance.StoredMealQualityTotal = StoredMealQualityTotal;
	InOutSnapshot.Sustenance.LastServedAverageMealQuality = LastServedAverageMealQuality;
	InOutSnapshot.Sustenance.CurrentDailyMealDemandUnits = CurrentDailyMealDemandUnits;
	InOutSnapshot.Sustenance.LastKnownPopulation = LastKnownPopulation;
	InOutSnapshot.Sustenance.LastMealCoverage = LastMealCoverage;
	InOutSnapshot.Sustenance.bFoodShortage = bFoodShortage;
	InOutSnapshot.Sustenance.MealServiceElapsedSeconds = MealServiceElapsedSeconds;
}

void UWS_Sustenance::ApplySnapshot_Implementation(const FEidosWorldSnapshot& Snapshot)
{
	StoredMealUnits = FMath::Max(0.f, Snapshot.Sustenance.StoredMealUnits);
	StoredMealQualityTotal = FMath::Max(0.f, Snapshot.Sustenance.StoredMealQualityTotal);
	LastServedAverageMealQuality = FMath::Max(0.f, Snapshot.Sustenance.LastServedAverageMealQuality);
	CurrentDailyMealDemandUnits = FMath::Max(0.f, Snapshot.Sustenance.CurrentDailyMealDemandUnits);
	LastKnownPopulation = FMath::Max(0, Snapshot.Sustenance.LastKnownPopulation);
	LastMealCoverage = FMath::Clamp(Snapshot.Sustenance.LastMealCoverage, 0.f, 1.f);
	bFoodShortage = Snapshot.Sustenance.bFoodShortage;
	MealServiceElapsedSeconds = FMath::Max(0.f, Snapshot.Sustenance.MealServiceElapsedSeconds);

	bDirty = true;
}

float UWS_Sustenance::GetStoredAverageMealQuality() const
{
	return ComputeWeightedAverage(StoredMealUnits, StoredMealQualityTotal);
}

float UWS_Sustenance::ComputeWeightedAverage(float InStoredUnits, float InStoredQualityTotal) const
{
	if (InStoredUnits <= KINDA_SMALL_NUMBER)
	{
		return 0.f;
	}

	return InStoredQualityTotal / InStoredUnits;
}

void UWS_Sustenance::RefreshDemandFromPopulation()
{
	UWS_Population* Population = GetWorld() ? GetWorld()->GetSubsystem<UWS_Population>() : nullptr;
	if (!Population)
	{
		LastKnownPopulation = 0;
		CurrentDailyMealDemandUnits = 0.f;
		return;
	}

	int32 FriendlyCount = 0;
	for (const TWeakObjectPtr<APageCharacter>& WeakPage : Population->GetOwnedPages())
	{
		if (const APageCharacter* Page = WeakPage.Get())
		{
			if (Page->IsFriendly())
			{
				++FriendlyCount;
			}
		}
	}

	LastKnownPopulation = FriendlyCount;
	CurrentDailyMealDemandUnits = FriendlyCount * DailyMealDemandPerPage;
}

const FItemDefinitionRow* UWS_Sustenance::FindItemDefinition(FName ItemId) const
{
	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UGIS_DataRegistry* Registry = GameInstance ? GameInstance->GetSubsystem<UGIS_DataRegistry>() : nullptr;
	return Registry && Registry->EnsureReadySync() ? Registry->GetItemDef(ItemId) : nullptr;
}

void UWS_Sustenance::RefreshMealStorageFromWarehouse()
{
	UWS_ItemStorage* Storage = GetWorld() ? GetWorld()->GetSubsystem<UWS_ItemStorage>() : nullptr;
	if (!Storage)
	{
		return;
	}

	float NewUnits = 0.f;
	float NewQualityTotal = 0.f;
	for (const FItemStack& Stack : Storage->GetStoredItems())
	{
		const FItemDefinitionRow* Def = FindItemDefinition(Stack.ItemId);
		if (!Def || Def->SettlementMealUnits <= 0.f || Stack.Quantity <= 0)
		{
			continue;
		}

		NewUnits += Def->SettlementMealUnits * Stack.Quantity;
		const float StackQuality = Stack.TotalQuality > KINDA_SMALL_NUMBER
			? Stack.TotalQuality
			: Def->DefaultMealQuality * Stack.Quantity;
		NewQualityTotal += StackQuality * Def->SettlementMealUnits;
	}

	if (!FMath::IsNearlyEqual(StoredMealUnits, NewUnits) || !FMath::IsNearlyEqual(StoredMealQualityTotal, NewQualityTotal))
	{
		StoredMealUnits = NewUnits;
		StoredMealQualityTotal = NewQualityTotal;
		bDirty = true;
	}
}

void UWS_Sustenance::ServeSettlementMeal()
{
	const float Demand = CurrentDailyMealDemandUnits;
	if (Demand <= KINDA_SMALL_NUMBER)
	{
		LastMealCoverage = 1.f;
		bFoodShortage = false;
		bDirty = true;
		return;
	}

	UWS_ItemStorage* Storage = GetWorld() ? GetWorld()->GetSubsystem<UWS_ItemStorage>() : nullptr;
	if (!Storage)
	{
		return;
	}

	float RemainingDemand = Demand;
	float ServedUnits = 0.f;
	float ServedQualityTotal = 0.f;
	const TArray<FItemStack> StoredStacks = Storage->GetStoredItems();
	for (const FItemStack& Stack : StoredStacks)
	{
		if (RemainingDemand <= KINDA_SMALL_NUMBER)
		{
			break;
		}

		const FItemDefinitionRow* Def = FindItemDefinition(Stack.ItemId);
		if (!Def || Def->SettlementMealUnits <= 0.f || Stack.Quantity <= 0)
		{
			continue;
		}

		const int32 RequiredItems = FMath::CeilToInt(RemainingDemand / Def->SettlementMealUnits);
		float RemovedQuality = 0.f;
		const int32 RemovedItems = Storage->TryTakeStoredItem(Stack.ItemId, FMath::Min(Stack.Quantity, RequiredItems), RemovedQuality);
		if (RemovedItems <= 0)
		{
			continue;
		}

		const float Units = RemovedItems * Def->SettlementMealUnits;
		const float QualityTotal = RemovedQuality > KINDA_SMALL_NUMBER
			? RemovedQuality * Def->SettlementMealUnits
			: Def->DefaultMealQuality * Units;
		ServedUnits += Units;
		ServedQualityTotal += QualityTotal;
		RemainingDemand = FMath::Max(0.f, RemainingDemand - Units);
	}

	LastMealCoverage = FMath::Clamp(ServedUnits / Demand, 0.f, 1.f);
	bFoodShortage = LastMealCoverage < (1.f - KINDA_SMALL_NUMBER);
	LastServedAverageMealQuality = ServedUnits > KINDA_SMALL_NUMBER
		? ServedQualityTotal / ServedUnits
		: 0.f;
	RefreshMealStorageFromWarehouse();
	bDirty = true;

	UE_LOG(LogTemp, Log, TEXT("[Sustenance] Meal service Demand=%.2f Served=%.2f Coverage=%.0f%% Quality=%.2f%s"),
		Demand,
		ServedUnits,
		LastMealCoverage * 100.f,
		LastServedAverageMealQuality,
		bFoodShortage ? TEXT(" SHORTAGE") : TEXT(""));
}
