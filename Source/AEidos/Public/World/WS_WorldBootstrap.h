// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WS_WorldBootstrap.generated.h"

/**
 * 
 */
UCLASS()
class AEIDOS_API UWS_WorldBootstrap : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	bool IsGameWorld(const UWorld* World) const;
	void BeginBootstrap();
	void ScheduleBootstrapNextTick();

private:
	UPROPERTY(Transient)
	TObjectPtr<UWorld> CachedWorld = nullptr;

	bool bBootstrapScheduled = false;
	bool bBootstrapped = false;
	
};
