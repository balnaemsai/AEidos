// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "PageCharacter.generated.h"

class UStatsComponent;
class UInputMappingContext;
class UInputAction;

UCLASS()
class AEIDOS_API APageCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	APageCharacter();

	UFUNCTION(BlueprintCallable, Category="Page")
	UStatsComponent* GetStats() const { return Stats; }

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	void HandleMove(const FInputActionValue& Value);
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	UStatsComponent* Stats;
	
	UPROPERTY(EditDefaultsOnly, Category="Input")
	UInputMappingContext* PageInputMappingContext;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	UInputAction* MoveAction;
	
};
