// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Save/SaveGameParticipant.h"
#include "Simulation/SimSystem.h"
#include "Subsystems/WorldSubsystem.h"
#include "WS_Sustenance.generated.h"

class UWS_Population;
class UWS_ItemStorage;
class USimCommandBuffer;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSustenanceChanged);

/**
 * Settlement-wide meal service. Prepared meal items in the warehouse are the
 * single source of truth; this subsystem serves them periodically and records
 * only the resulting quality and shortage state.
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

	/** 0..1 portion of the previous settlement meal demand that was met. */
	UFUNCTION(BlueprintPure, Category="Sustenance")
	float GetLastMealCoverage() const { return LastMealCoverage; }

	UFUNCTION(BlueprintPure, Category="Sustenance")
	bool HasFoodShortage() const { return bFoodShortage; }

	UFUNCTION(BlueprintPure, Category="Sustenance")
	float GetSecondsUntilNextMealService() const { return FMath::Max(0.f, MealServiceIntervalSeconds - MealServiceElapsedSeconds); }

	UPROPERTY(BlueprintAssignable, Category="Sustenance")
	FOnSustenanceChanged OnSustenanceChanged;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sustenance|Tuning", meta=(ClampMin="0.0"))
	float DailyMealDemandPerPage = 1.f;

	/** One game-day meal service in the current prototype. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sustenance|Tuning", meta=(ClampMin="1.0"))
	float MealServiceIntervalSeconds = 120.f;

private:
	float ComputeWeightedAverage(float InStoredUnits, float InStoredQualityTotal) const;
	void RefreshDemandFromPopulation();
	void RefreshMealStorageFromWarehouse();
	void ServeSettlementMeal();
	void ApplyCurrentSustenanceStateToPages(float FixedDeltaSeconds);
	const struct FItemDefinitionRow* FindItemDefinition(FName ItemId) const;

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

	UPROPERTY()
	float LastMealCoverage = 1.f;

	UPROPERTY()
	bool bFoodShortage = false;

	UPROPERTY()
	float MealServiceElapsedSeconds = 0.f;

	bool bDirty = false;
};
