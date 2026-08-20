#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WS_SettlementValue.generated.h"

/**
 * Centralized, deliberately provisional settlement-value model. The detailed
 * Page and asset valuation formulas can evolve here without changing portal,
 * dungeon, or raid systems.
 */
UCLASS()
class AEIDOS_API UWS_SettlementValue : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="Settlement|Value")
	float GetCurrentSettlementValue() const;

	UFUNCTION(BlueprintPure, Category="Settlement|Value")
	float GetPageValue(const class APageCharacter* Page) const;

	UFUNCTION(BlueprintPure, Category="Settlement|Value")
	float GetDifficultyMean() const;

	/** Draws a bounded normal sample around the current settlement difficulty. */
	float RollDungeonDifficulty(int32 Seed) const;

private:
	float GetWarehouseResourceValue() const;
	float GetWarehouseItemValue() const;
};
