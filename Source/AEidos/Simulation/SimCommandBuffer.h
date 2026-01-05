// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SimCommandBuffer.generated.h"

/**
 * 
 */
UCLASS()
class AEIDOS_API USimCommandBuffer : public UObject
{
	GENERATED_BODY()

public:
	void Enqueue(TFunction<void()>&& cmd);

	void Flush();

	int32 Num() const { return Commands.Num(); }
	void Reset() { Commands.Reset(); }

private:
	TArray<TFunction<void()>> Commands;
	
};
