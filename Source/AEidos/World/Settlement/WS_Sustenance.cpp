// Fill out your copyright notice in the Description page of Project Settings.

#include "World/Settlement/WS_Sustenance.h"

#include "Entities/Page/PageCharacter.h"
#include "World/Settlement/WS_Population.h"

void UWS_Sustenance::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	MealBatches.Reset();
	StoredMealUnits = 0.f;
	StoredMealQualityTotal = 0.f;
	LastServedAverageMealQuality = 0.f;
	CurrentDailyMealDemandUnits = 0.f;
	LastKnownPopulation = 0;
	bDirty = false;
}

void UWS_Sustenance::SimPlan_Implementation(USimCommandBuffer* CommandBuffer, float FixedDeltaSeconds)
{
	RefreshDemandFromPopulation();
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
}

void UWS_Sustenance::ApplySnapshot_Implementation(const FEidosWorldSnapshot& Snapshot)
{
	StoredMealUnits = FMath::Max(0.f, Snapshot.Sustenance.StoredMealUnits);
	StoredMealQualityTotal = FMath::Max(0.f, Snapshot.Sustenance.StoredMealQualityTotal);
	LastServedAverageMealQuality = FMath::Max(0.f, Snapshot.Sustenance.LastServedAverageMealQuality);
	CurrentDailyMealDemandUnits = FMath::Max(0.f, Snapshot.Sustenance.CurrentDailyMealDemandUnits);
	LastKnownPopulation = FMath::Max(0, Snapshot.Sustenance.LastKnownPopulation);

	MealBatches.Reset();
	if (StoredMealUnits > 0.f)
	{
		FMealBatchRecord Aggregate;
		Aggregate.MealUnits = StoredMealUnits;
		Aggregate.MealQuality = GetStoredAverageMealQuality();
		Aggregate.SourceRecipeId = TEXT("RestoredAverage");
		MealBatches.Add(Aggregate);
	}

	bDirty = true;
}

void UWS_Sustenance::RegisterProducedMeals(float MealUnits, float MealQuality, FName SourceRecipeId)
{
	if (MealUnits <= 0.f)
	{
		return;
	}

	const float ClampedUnits = FMath::Max(0.f, MealUnits);
	const float ClampedQuality = FMath::Max(0.f, MealQuality);

	FMealBatchRecord Batch;
	Batch.MealUnits = ClampedUnits;
	Batch.MealQuality = ClampedQuality;
	Batch.SourceRecipeId = SourceRecipeId;
	MealBatches.Add(Batch);

	StoredMealUnits += ClampedUnits;
	StoredMealQualityTotal += (ClampedUnits * ClampedQuality);
	bDirty = true;
}

void UWS_Sustenance::ClearMealStorage()
{
	MealBatches.Reset();
	StoredMealUnits = 0.f;
	StoredMealQualityTotal = 0.f;
	LastServedAverageMealQuality = 0.f;
	bDirty = true;
}

float UWS_Sustenance::GetStoredAverageMealQuality() const
{
	return ComputeWeightedAverage(StoredMealUnits, StoredMealQualityTotal);
}

float UWS_Sustenance::PredictAverageMealQualityAfterAdd(float MealUnits, float MealQuality) const
{
	if (MealUnits <= 0.f)
	{
		return GetStoredAverageMealQuality();
	}

	const float NewUnits = StoredMealUnits + MealUnits;
	const float NewQualityTotal = StoredMealQualityTotal + (MealUnits * FMath::Max(0.f, MealQuality));
	return ComputeWeightedAverage(NewUnits, NewQualityTotal);
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
