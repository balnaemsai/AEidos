// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StatsComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStatsChanged);

USTRUCT(BlueprintType)
struct FPageStatsDelta
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HungerDelta = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FatigueDelta = 0.f;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class AEIDOS_API UStatsComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UStatsComponent();

	UFUNCTION(BlueprintCallable, Category="Stats")
	float GetHunger() const { return Hunger; }

	UFUNCTION(BlueprintCallable, Category="Stats")
	float GetFatigue() const { return Fatigue; }

	UFUNCTION(BlueprintCallable, Category="Stats")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintCallable, Category="Stats")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintCallable, Category="Stats")
	bool IsDead() const { return Health <= 0.f; }

	UFUNCTION(BlueprintCallable, Category="Stats|Combat")
	float GetCombatAgility() const { return CombatAgility; }

	UFUNCTION(BlueprintCallable, Category="Stats|Combat")
	float GetCombatActionThreshold() const { return CombatActionThreshold; }

	UFUNCTION(BlueprintCallable, Category="Stats|Combat")
	int32 GetCombatActionPointsPerTurn() const { return CombatActionPointsPerTurn; }

	// --- Mutations (Commit에서만 호출하는 걸 권장) ---
	UFUNCTION(BlueprintCallable, Category="Stats")
	void ApplyDelta(const FPageStatsDelta& Delta);

	UFUNCTION(BlueprintCallable, Category="Stats")
	void ApplyDamage(float DamageAmount);

	UFUNCTION(BlueprintCallable, Category="Stats")
	void RestoreToFull();

	UPROPERTY(BlueprintAssignable)
	FOnStatsChanged OnStatsChanged;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta=(ClampMin="0.0", ClampMax="100.0"))
	float Hunger = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta=(ClampMin="0.0", ClampMax="100.0"))
	float Fatigue = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta=(ClampMin="1.0"))
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta=(ClampMin="0.0"))
	float Health = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats|Combat", meta=(ClampMin="0.1"))
	float CombatAgility = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats|Combat", meta=(ClampMin="1.0"))
	float CombatActionThreshold = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats|Combat", meta=(ClampMin="1"))
	int32 CombatActionPointsPerTurn = 2;

	void ClampAll();
};
