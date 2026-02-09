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

	// --- Mutations (Commit에서만 호출하는 걸 권장) ---
	UFUNCTION(BlueprintCallable, Category="Stats")
	void ApplyDelta(const FPageStatsDelta& Delta);

	UPROPERTY(BlueprintAssignable)
	FOnStatsChanged OnStatsChanged;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta=(ClampMin="0.0", ClampMax="100.0"))
	float Hunger = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta=(ClampMin="0.0", ClampMax="100.0"))
	float Fatigue = 0.f;

	void ClampAll();
};
