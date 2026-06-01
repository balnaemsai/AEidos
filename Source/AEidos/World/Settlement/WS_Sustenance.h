// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Save/SaveGameParticipant.h"
#include "Simulation/SimSystem.h"
#include "Subsystems/WorldSubsystem.h"
#include "WS_Sustenance.generated.h"

class UWS_Population;
class USimCommandBuffer;

USTRUCT(BlueprintType)
struct FMealBatchRecord
{
	GENERATED_BODY()

	// A stack-like quantity of prepared meals contributed at the same quality band.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Meal")
	float MealUnits = 0.f;

	// Final cooked meal quality after cook skill + ingredient quality/value are resolved.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Meal")
	float MealQuality = 0.f;

	// Optional future hook for recipe/output tracking.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Meal")
	FName SourceRecipeId;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSustenanceChanged);

/**
 * Settlement-wide meal storage and average quality model.
 * This intentionally does not track who ate what yet.
 * Cooking jobs will convert ingredient inputs into prepared meal units and register them here.
 */
UCLASS()
class AEIDOS_API UWS_Sustenance : public UWorldSubsystem, public ISimSystem, public ISaveGameParticipant
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// ISimSystem
	virtual void SimPlan_Implementation(USimCommandBuffer* CommandBuffer, float FixedDeltaSeconds) override;
	virtual void SimCommit_Implementation(USimCommandBuffer* CommandBuffer, float FixedDeltaSeconds) override {}
	virtual void SimPost_Implementation(float FixedDeltaSeconds) override;
	virtual int32 GetSimOrder_Implementation() const override { return 95; }

	// SaveGameParticipant
	virtual void WriteToSnapshot_Implementation(FEidosWorldSnapshot& InOutSnapshot) const override;
	virtual void ApplySnapshot_Implementation(const FEidosWorldSnapshot& Snapshot) override;

	UFUNCTION(BlueprintCallable, Category="Sustenance")
	void RegisterProducedMeals(float MealUnits, float MealQuality, FName SourceRecipeId = NAME_None);

	UFUNCTION(BlueprintCallable, Category="Sustenance")
	void ClearMealStorage();

	UFUNCTION(BlueprintPure, Category="Sustenance")
	float GetStoredMealUnits() const { return StoredMealUnits; }

	UFUNCTION(BlueprintPure, Category="Sustenance")
	float GetStoredAverageMealQuality() const;

	UFUNCTION(BlueprintPure, Category="Sustenance")
	float GetLastServedAverageMealQuality() const { return LastServedAverageMealQuality; }

	UFUNCTION(BlueprintPure, Category="Sustenance")
	float GetCurrentDailyMealDemandUnits() const { return CurrentDailyMealDemandUnits; }

	UFUNCTION(BlueprintPure, Category="Sustenance")
	int32 GetLastKnownPopulation() const { return LastKnownPopulation; }

	UFUNCTION(BlueprintPure, Category="Sustenance")
	const TArray<FMealBatchRecord>& GetMealBatches() const { return MealBatches; }

	// Future-facing helper for UI/tooling: what would the weighted average be after adding a new cooked batch?
	UFUNCTION(BlueprintPure, Category="Sustenance")
	float PredictAverageMealQualityAfterAdd(float MealUnits, float MealQuality) const;

	UPROPERTY(BlueprintAssignable, Category="Sustenance")
	FOnSustenanceChanged OnSustenanceChanged;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sustenance|Tuning", meta=(ClampMin="0.0"))
	float DailyMealDemandPerPage = 1.f;

private:
	float ComputeWeightedAverage(float InStoredUnits, float InStoredQualityTotal) const;
	void RefreshDemandFromPopulation();

	UPROPERTY()
	TArray<FMealBatchRecord> MealBatches;

	UPROPERTY()
	float StoredMealUnits = 0.f;

	UPROPERTY()
	float StoredMealQualityTotal = 0.f;

	UPROPERTY()
	float LastServedAverageMealQuality = 0.f;

	UPROPERTY()
	float CurrentDailyMealDemandUnits = 0.f;

	UPROPERTY()
	int32 LastKnownPopulation = 0;

	bool bDirty = false;
};
