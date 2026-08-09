// Fill out your copyright notice in the Description page of Project Settings.


#include "Entities/Page/Components/StatsComponent.h"

// Sets default values for this component's properties
UStatsComponent::UStatsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UStatsComponent::ClampAll()
{
	Hunger  = FMath::Clamp(Hunger,  0.f, 100.f);
	Fatigue = FMath::Clamp(Fatigue, 0.f, 100.f);
	MaxHealth = FMath::Max(1.f, MaxHealth);
	Health = FMath::Clamp(Health, 0.f, MaxHealth);
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
	if (DamageAmount <= 0.f)
	{
		return;
	}

	Health -= DamageAmount;
	ClampAll();
	OnStatsChanged.Broadcast();
}

float UStatsComponent::RestoreHealth(float RequestedAmount)
{
	if (RequestedAmount <= 0.f || Health <= 0.f || Health >= MaxHealth)
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
	OnStatsChanged.Broadcast();
}

void UStatsComponent::RestoreToFull()
{
	Health = MaxHealth;
	ClampAll();
	OnStatsChanged.Broadcast();
}
