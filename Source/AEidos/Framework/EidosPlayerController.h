// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "EidosPlayerController.generated.h"

class UCameraModeComponent;
class UInputMappingContext;
class UInputAction;
class APageCharacter;

/**
 * 
 */
UCLASS()
class AEIDOS_API AEidosPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AEidosPlayerController();
	
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	void HandleWorldSimReady();

	void OnToggleView(const FInputActionValue& Value);      // V
	void OnToggleControlMode(const FInputActionValue& Value); // Y

	APageCharacter* FindAnyPage() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
	UCameraModeComponent* CameraMode;

	// 공통 단축키(IMC)
	UPROPERTY(EditDefaultsOnly, Category="Input")
	UInputMappingContext* CommonIMC;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	UInputAction* ToggleViewAction;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	UInputAction* ToggleControlModeAction;

	UFUNCTION()
	void OnLook(const FInputActionValue& Value);

	UPROPERTY(EditDefaultsOnly, Category="Input")
	UInputAction* LookAction; // IA_Look
	
};
