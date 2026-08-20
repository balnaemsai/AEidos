// Fill out your copyright notice in the Description page of Project Settings.


#include "Entities/Page/Components/StatsComponent.h"

// Sets default values for this component's properties
UStatsComponent::UStatsComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UStatsComponent::ClampAll()
{
	Hunger  = FMath::Clamp(Hunger,  0.f, 100.f);
	Fatigue = FMath::Clamp(Fatigue, 0.f, 100.f);
	MaxHealth = FMath::Max(1.f, MaxHealth);
	Health = FMath::Clamp(Health, 0.f, MaxHealth);
	DownedDeathDelaySeconds = FMath::Max(1.f, DownedDeathDelaySeconds);
	DownedTimeRemaining = FMath::Max(0.f, DownedTimeRemaining);
	RescueRecoveryDurationSeconds = FMath::Max(0.1f, RescueRecoveryDurationSeconds);
	RecoveryTimeRemaining = FMath::Max(0.f, RecoveryTimeRemaining);
	CombatAgility = FMath::Max(0.1f, CombatAgility);
	CombatActionThreshold = FMath::Max(1.f, CombatActionThreshold);
	CombatActionPointsPerTurn = FMath::Max(1, CombatActionPointsPerTurn);
}

void UStatsComponent::ApplyDelta(const FPageStatsDelta& Delta)
{
	Hunger  += Delta.HungerDelta;
	Fatigue += Delta.FatigueDelta;

	ClampAll();
	OnStatsChanged.Broadcast();
}

void UStatsComponent::ApplyDamage(float DamageAmount)
{
	if (DamageAmount <= 0.f || IsDead())
	{
		return;
	}

	// A further hit on an incapacitated Page is intentionally lethal. Normal
	// combat removes downed units first, so this is reserved for later hazards
	// or an explicit finishing action.
	if (IsDowned())
	{
		MarkPermanentlyDead();
		return;
	}

	Health -= DamageAmount;
	ClampAll();
	if (Health <= 0.f)
	{
		VitalState = EPageVitalState::Downed;
		DownedTimeRemaining = DownedDeathDelaySeconds;
		RecoveryTimeRemaining = 0.f;
		UE_LOG(LogTemp, Log, TEXT("[Stats] Page downed Owner=%s Component=%s Delay=%.0fs"),
			*GetNameSafe(GetOwner()), *GetName(), DownedTimeRemaining);
	}
	OnStatsChanged.Broadcast();
}

float UStatsComponent::ApplyNonLethalDamage(float DamageAmount, float MinimumHealthFraction)
{
	if (DamageAmount <= 0.f || IsDead() || IsDowned())
	{
		return 0.f;
	}

	const float MinimumHealth = FMath::Clamp(MinimumHealthFraction, 0.f, 1.f) * MaxHealth;
	const float PreviousHealth = Health;
	Health = FMath::Max(MinimumHealth, Health - DamageAmount);
	const float AppliedDamage = PreviousHealth - Health;
	if (AppliedDamage > KINDA_SMALL_NUMBER)
	{
		OnStatsChanged.Broadcast();
	}

	return AppliedDamage;
}

float UStatsComponent::RestoreHealth(float RequestedAmount)
{
	if (RequestedAmount <= 0.f || IsDowned() || IsDead() || Health >= MaxHealth)
	{
		return 0.f;
	}

	const float PreviousHealth = Health;
	Health = FMath::Min(MaxHealth, Health + RequestedAmount);
	const float RestoredAmount = Health - PreviousHealth;
	if (RestoredAmount > 0.f)
	{
		OnStatsChanged.Broadcast();
	}
	return RestoredAmount;
}

void UStatsComponent::Revive(float NewHealth)
{
	Health = FMath::Clamp(NewHealth, 1.f, FMath::Max(1.f, MaxHealth));
	VitalState = EPageVitalState::Alive;
	DownedTimeRemaining = 0.f;
	RecoveryTimeRemaining = 0.f;
	OnStatsChanged.Broadcast();
}

void UStatsComponent::ReviveFromRescue(float NewHealth)
{
	Health = FMath::Clamp(NewHealth, 1.f, FMath::Max(1.f, MaxHealth));
	VitalState = EPageVitalState::Recovering;
	DownedTimeRemaining = 0.f;
	RecoveryTimeRemaining = RescueRecoveryDurationSeconds;
	OnStatsChanged.Broadcast();
}

void UStatsComponent::RestoreToFull()
{
	Health = MaxHealth;
	VitalState = EPageVitalState::Alive;
	DownedTimeRemaining = 0.f;
	RecoveryTimeRemaining = 0.f;
	ClampAll();
	OnStatsChanged.Broadcast();
}

void UStatsComponent::MarkPermanentlyDead()
{
	if (IsDead())
	{
		return;
	}

	Health = 0.f;
	VitalState = EPageVitalState::Dead;
	DownedTimeRemaining = 0.f;
	RecoveryTimeRemaining = 0.f;
	UE_LOG(LogTemp, Warning, TEXT("[Stats] Page permanently died Owner=%s Component=%s"),
		*GetNameSafe(GetOwner()), *GetName());
	OnStatsChanged.Broadcast();
}

void UStatsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (IsDowned())
	{
		const float PreviousWholeSeconds = FMath::CeilToFloat(DownedTimeRemaining);
		DownedTimeRemaining = FMath::Max(0.f, DownedTimeRemaining - FMath::Max(0.f, DeltaTime));
		if (DownedTimeRemaining <= 0.f)
		{
			MarkPermanentlyDead();
			return;
		}

		if (FMath::CeilToFloat(DownedTimeRemaining) != PreviousWholeSeconds)
		{
			OnStatsChanged.Broadcast();
		}
		return;
	}

	if (!IsRecovering())
	{
		return;
	}

	const float PreviousWholeSeconds = FMath::CeilToFloat(RecoveryTimeRemaining);
	RecoveryTimeRemaining = FMath::Max(0.f, RecoveryTimeRemaining - FMath::Max(0.f, DeltaTime));
	if (RecoveryTimeRemaining <= 0.f)
	{
		VitalState = EPageVitalState::Alive;
		OnStatsChanged.Broadcast();
		return;
	}

	if (FMath::CeilToFloat(RecoveryTimeRemaining) != PreviousWholeSeconds)
	{
		OnStatsChanged.Broadcast();
	}
}

void UStatsComponent::ApplyDifficultyScale(float Difficulty)
{
	const float SafeDifficulty = FMath::Max(0.5f, Difficulty);
	const float HealthScale = 1.f + (SafeDifficulty - 1.f) * 0.45f;
	const float AgilityScale = 1.f + (SafeDifficulty - 1.f) * 0.12f;
	MaxHealth *= FMath::Max(0.5f, HealthScale);
	Health = MaxHealth;
	CombatAgility *= FMath::Max(0.5f, AgilityScale);
	ClampAll();
	OnStatsChanged.Broadcast();
}
