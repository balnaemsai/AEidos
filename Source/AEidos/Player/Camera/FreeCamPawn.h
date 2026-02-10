// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "FreeCamPawn.generated.h"

class UCameraComponent;
class UFloatingPawnMovement;
class UInputMappingContext;
class USpringArmComponent;
class UInputAction;

UCLASS()
class AEIDOS_API AFreeCamPawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AFreeCamPawn();

	UInputMappingContext* GetFreeCamIMC() const { return FreeCamIMC; }

protected:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	void HandleMove(const FInputActionValue& Value);
	void HandleLook(const FInputActionValue& Value);

protected:
	UPROPERTY(VisibleAnywhere)
	UCameraComponent* Camera;

	UPROPERTY(VisibleAnywhere)
	UFloatingPawnMovement* Movement;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	UInputMappingContext* FreeCamIMC;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	UInputAction* MoveAction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
	USpringArmComponent* SpringArm;
};
