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
class AConstructionSiteActor;

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
	virtual void Tick(float DeltaTime) override;

	void HandleWorldSimReady();

	void OnToggleView(const FInputActionValue& Value);      // V
	void OnToggleControlMode(const FInputActionValue& Value); // Y

	APageCharacter* FindAnyPage() const;

	UCameraModeComponent* GetCameraMode() {return CameraMode;}

	UFUNCTION()
	void BeginBuildPlacement(FName BuildingId);

	UFUNCTION()
	void CancelBuildPlacement();

	UFUNCTION()
	bool IsInPlacementMode() const;

	UFUNCTION()
	FName GetPendingBuildingId() const;

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

	UFUNCTION()
	void OnOrbitYaw(const FInputActionValue& Value);

	UFUNCTION()
	void OnZoom(const FInputActionValue& Value);

	UFUNCTION()
	void OnPrimaryClick(const FInputActionValue& Value);

	UPROPERTY(EditDefaultsOnly, Category="Input")
	UInputAction* LookAction; // IA_Look

	UPROPERTY(EditDefaultsOnly, Category="Input")
	UInputAction* IA_OrbitYaw; 

	UPROPERTY(EditDefaultsOnly, Category="Input")
	UInputAction* IA_Zoom;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> IA_PrimaryClick;

	void ConfirmBuildPlacement();
	void UpdateBuildPreview();
	void SpawnOrRefreshBuildPreview();

	UPROPERTY(Transient)
	bool bInBuildPlacementMode = false;

	UPROPERTY(Transient)
	FName PendingBuildingId = NAME_None;

	UPROPERTY(Transient)
	TWeakObjectPtr<AConstructionSiteActor> BuildPreviewActor;
	
};
