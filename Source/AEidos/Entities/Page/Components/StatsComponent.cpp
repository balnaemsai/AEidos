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
}

void UStatsComponent::ApplyDelta(const FPageStatsDelta& Delta)
{
	Hunger  += Delta.HungerDelta;
	Fatigue += Delta.FatigueDelta;

	ClampAll();
	OnStatsChanged.Broadcast();
}