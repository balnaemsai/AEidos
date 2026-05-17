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
class APortalActor;
class AActor;
class ATerritoryChunkActor;
class UWS_Population;

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
	APageCharacter* GetSelectedPage() const;

	UFUNCTION(BlueprintCallable, Category="Page")
	bool SelectPageByEntityId(int32 PageId);

	UCameraModeComponent* GetCameraMode() {return CameraMode;}

	UFUNCTION()
	void BeginBuildPlacement(FName BuildingId);

	UFUNCTION()
	void CancelBuildPlacement();

	UFUNCTION()
	void BeginTerritoryExpansionPlacement();

	UFUNCTION()
	void CancelTerritoryExpansionPlacement();

	UFUNCTION()
	bool IsInPlacementMode() const;

	UFUNCTION()
	bool IsInTerritoryPlacementMode() const;

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

	UFUNCTION()
	void OnInteractPressed();

	UFUNCTION()
	void OnToggleFirstPersonUIFocus();

	UFUNCTION()
	void OnSelectPreviousPage();

	UFUNCTION()
	void OnSelectNextPage();

	UFUNCTION()
	void OnEndTurnPressed();

	UFUNCTION()
	void OnCombatActionSlot1();

	UFUNCTION()
	void OnCombatActionSlot2();

	UFUNCTION()
	void OnCombatActionSlot3();

	UFUNCTION()
	void OnCombatActionSlot4();

	UFUNCTION()
	void OnCombatActionSlot5();

	UFUNCTION()
	void OnCombatActionSlot6();

	UFUNCTION()
	void OnCombatActionSlot7();

	UFUNCTION()
	void OnCombatActionSlot8();

	UFUNCTION()
	void OnCombatActionSlot9();

	UFUNCTION()
	void OnCombatActionSlot0();

	void ApplyHybridInputMode();
	void ApplyFirstPersonInputMode();
	void RefreshInputModeForCurrentContext();
	bool IsUsingFirstPersonGameplayInput() const;

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
	void ConfirmTerritoryExpansionPlacement();
	void UpdateTerritoryPreview();
	void SpawnOrRefreshTerritoryPreview();
	APageCharacter* FindFocusedHostileCombatTarget() const;
	AActor* FindFocusedInteractActor() const;
	bool TryInteractWithActor(AActor* TargetActor);
	void SelectAdjacentPage(int32 Direction);
	void EnsureValidSelectedPage();
	void TriggerCombatActionSlot(int32 SlotIndex);

	UPROPERTY(Transient)
	bool bInBuildPlacementMode = false;

	UPROPERTY(Transient)
	FName PendingBuildingId = NAME_None;

	UPROPERTY(Transient)
	TWeakObjectPtr<AConstructionSiteActor> BuildPreviewActor;

	UPROPERTY(Transient)
	bool bInTerritoryPlacementMode = false;

	UPROPERTY(Transient)
	TWeakObjectPtr<ATerritoryChunkActor> TerritoryPreviewActor;

	UPROPERTY(Transient)
	FIntPoint PendingTerritoryCoord = FIntPoint::ZeroValue;

	UPROPERTY(EditDefaultsOnly, Category="Interaction")
	float InteractSearchRadius = 300.f;

	UPROPERTY(EditDefaultsOnly, Category="Interaction")
	float InteractMaxDistance = 1200.f;

	UPROPERTY(EditDefaultsOnly, Category="Interaction")
	float InteractForwardDotThreshold = 0.55f;

	UPROPERTY(Transient)
	bool bFirstPersonUIFocusMode = false;
	
};
