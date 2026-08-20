// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StatsComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStatsChanged);

UENUM(BlueprintType)
enum class EPageVitalState : uint8
{
	Alive,
	Downed,
	Recovering,
	Dead
};

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
	bool IsDead() const { return VitalState == EPageVitalState::Dead; }

	/** A downed Page is alive but cannot move, work, or participate in combat. */
	UFUNCTION(BlueprintCallable, Category="Stats")
	bool IsDowned() const { return VitalState == EPageVitalState::Downed; }

	/** A rescued Page is conscious but cannot act until its recovery timer expires. */
	UFUNCTION(BlueprintCallable, Category="Stats")
	bool IsRecovering() const { return VitalState == EPageVitalState::Recovering; }

	UFUNCTION(BlueprintCallable, Category="Stats")
	EPageVitalState GetVitalState() const { return VitalState; }

	UFUNCTION(BlueprintCallable, Category="Stats")
	float GetDownedTimeRemaining() const { return DownedTimeRemaining; }

	UFUNCTION(BlueprintCallable, Category="Stats")
	float GetRecoveryTimeRemaining() const { return RecoveryTimeRemaining; }

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

	/** Reduces health without creating a downed/death state, stopping at the supplied max-health fraction. */
	UFUNCTION(BlueprintCallable, Category="Stats")
	float ApplyNonLethalDamage(float DamageAmount, float MinimumHealthFraction = 0.01f);

	/** Restores health without exceeding MaxHealth and returns the amount actually restored. */
	UFUNCTION(BlueprintCallable, Category="Stats")
	float RestoreHealth(float RequestedAmount);

	/** Restores a defeated Page for explicit systems such as capture or recruitment. */
	UFUNCTION(BlueprintCallable, Category="Stats")
	void Revive(float NewHealth);

	/** Restores a downed Page into a temporary no-action recovery state. */
	UFUNCTION(BlueprintCallable, Category="Stats")
	void ReviveFromRescue(float NewHealth);

	UFUNCTION(BlueprintCallable, Category="Stats")
	void RestoreToFull();

	/** Immediately converts a downed Page into a permanent death. */
	UFUNCTION(BlueprintCallable, Category="Stats")
	void MarkPermanentlyDead();

	/** Applies a one-time hostile difficulty multiplier after an actor is spawned. */
	void ApplyDifficultyScale(float Difficulty);

	UPROPERTY(BlueprintAssignable)
	FOnStatsChanged OnStatsChanged;

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta=(ClampMin="0.0", ClampMax="100.0"))
	float Hunger = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta=(ClampMin="0.0", ClampMax="100.0"))
	float Fatigue = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta=(ClampMin="1.0"))
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta=(ClampMin="0.0"))
	float Health = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats|Downed", meta=(ClampMin="1.0"))
	float DownedDeathDelaySeconds = 90.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Stats|Downed")
	EPageVitalState VitalState = EPageVitalState::Alive;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Stats|Downed")
	float DownedTimeRemaining = 0.f;

	/** Recovery duration applied when a Page is rescued from the downed state. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats|Recovery", meta=(ClampMin="0.1"))
	float RescueRecoveryDurationSeconds = 30.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Stats|Recovery")
	float RecoveryTimeRemaining = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats|Combat", meta=(ClampMin="0.1"))
	float CombatAgility = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats|Combat", meta=(ClampMin="1.0"))
	float CombatActionThreshold = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats|Combat", meta=(ClampMin="1"))
	int32 CombatActionPointsPerTurn = 2;

	void ClampAll();
};
