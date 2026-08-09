// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "World/Interaction/WorldInteractionTypes.h"
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
class UWS_CombatDirector;
class UWorldInteractionRadialWidget;
class UWorldInteractionFocusWidget;
class AWorldBlockActor;

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

	/** Starts direct placement of one block item held by the selected Page. */
	UFUNCTION(BlueprintCallable, Category="World Block")
	bool BeginBlockPlacement(FName ItemId);

	UFUNCTION(BlueprintCallable, Category="World Block")
	void CancelBlockPlacement();

	UFUNCTION(BlueprintPure, Category="World Block")
	bool IsInBlockPlacementMode() const { return bInBlockPlacementMode; }

	UFUNCTION()
	FName GetPendingBuildingId() const;

	UFUNCTION(BlueprintPure, Category="Combat")
	AActor* GetSelectedCombatTarget() const { return SelectedCombatTarget.Get(); }

	UFUNCTION(BlueprintPure, Category="Combat")
	int32 GetPendingCombatActionSlot() const { return PendingCombatActionSlot; }

	UFUNCTION(BlueprintPure, Category="Combat")
	FText GetCombatTargetingHint() const { return CombatTargetingHint; }

	UFUNCTION(BlueprintCallable, Category="Combat")
	void ClearSelectedCombatTarget();

	UFUNCTION(BlueprintCallable, Category="Combat")
	bool SelectCombatTarget(AActor* TargetActor);

	UFUNCTION(BlueprintCallable, Category="Combat")
	bool UseCombatActionSlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category="Combat")
	bool RequestCombatEndTurn();

	UFUNCTION(BlueprintPure, Category="World Interaction")
	AActor* GetContextInteractionTarget() const { return ContextInteractionTarget.Get(); }

	UFUNCTION(BlueprintPure, Category="World Interaction")
	const TArray<FWorldInteractionOption>& GetContextInteractionOptions() const { return ContextInteractionOptions; }

	UFUNCTION(BlueprintCallable, Category="World Interaction")
	bool ExecuteContextWorldInteraction(FName InteractionId);

	/** Arms an interaction selected in the radial menu. The next primary click performs it. */
	UFUNCTION(BlueprintCallable, Category="World Interaction")
	bool SelectContextWorldInteraction(FName InteractionId);

	UFUNCTION(BlueprintCallable, Category="World Interaction")
	void CloseWorldInteractionRadial();

	UFUNCTION(BlueprintImplementableEvent, Category="World Interaction")
	void OnWorldInteractionRadialRequested(AActor* TargetActor, const TArray<FWorldInteractionOption>& Options);

	UFUNCTION(BlueprintImplementableEvent, Category="World Interaction")
	void OnWorldInteractionRadialClosed();

	UFUNCTION(BlueprintImplementableEvent, Category="Combat")
	void OnSelectedCombatTargetChanged(AActor* NewTarget);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
	UCameraModeComponent* CameraMode;

	// 怨듯넻 ?⑥텞??IMC)
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
	void OnPrimaryClickHeld(const FInputActionValue& Value);
	void OnPrimaryClickCompleted(const FInputActionValue& Value);
	void OnSecondaryClick();

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
	void ConfirmBlockPlacement();
	void UpdateBlockPreview();
	void SpawnOrRefreshBlockPreview();
	AActor* FindFocusedCombatActionTarget() const;
	AActor* FindFocusedInteractActor() const;
	AActor* FindFocusedWorldInteractionActor() const;
	bool TryInteractWithActor(AActor* TargetActor);
	bool OpenWorldInteractionRadial(AActor* TargetActor);
	bool ExecuteDefaultWorldInteraction(AActor* TargetActor);
	bool ExecuteSelectedWorldInteraction();
	bool ShowWorldInteractionRadialWidget(AActor* TargetActor, const TArray<FWorldInteractionOption>& Options);
	bool ResolvePreparedWorldInteraction(AActor* TargetActor, TArray<FWorldInteractionOption>& OutOptions,
		FWorldInteractionOption& OutPreparedOption) const;
	void UpdateWorldInteractionFocus();
	void ClearWorldInteractionFocus();
	FText FormatPreparedWorldInteraction(const FWorldInteractionOption& Option) const;
	void ClearSelectedWorldInteraction();
	void SelectAdjacentPage(int32 Direction);
	void EnsureValidSelectedPage();
	bool TriggerCombatActionSlot(int32 SlotIndex);
	bool ExecutePendingCombatAction(APageCharacter* SelectedPage, UWS_CombatDirector* CombatDirector);
	void SetCombatTargetingHint(const FText& NewHint);
	void SetSelectedCombatTarget(AActor* NewTarget);

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
	bool bInBlockPlacementMode = false;

	UPROPERTY(Transient)
	FName PendingBlockItemId = NAME_None;

	UPROPERTY(Transient)
	TWeakObjectPtr<AWorldBlockActor> BlockPreviewActor;

	UPROPERTY(Transient)
	bool bBlockPlacementPreviewValid = false;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> SelectedCombatTarget;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> ContextInteractionTarget;

	UPROPERTY(Transient)
	TArray<FWorldInteractionOption> ContextInteractionOptions;

	// A radial option arms this action for the selected Page. It remains armed across
	// compatible focused blocks and falls back to each block's default when unavailable.
	UPROPERTY(Transient)
	TWeakObjectPtr<APageCharacter> SelectedWorldInteractionPage;

	UPROPERTY(Transient)
	FName SelectedWorldInteractionId = NAME_None;

	UPROPERTY(EditDefaultsOnly, Category="World Interaction")
	TSubclassOf<UWorldInteractionRadialWidget> WorldInteractionRadialClass;

	UPROPERTY(EditDefaultsOnly, Category="World Interaction")
	TSubclassOf<UWorldInteractionFocusWidget> WorldInteractionFocusClass;

	UPROPERTY(Transient)
	TObjectPtr<UWorldInteractionRadialWidget> ActiveWorldInteractionRadial;

	UPROPERTY(Transient)
	TObjectPtr<UWorldInteractionFocusWidget> ActiveWorldInteractionFocus;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> FocusedWorldInteractionActor;

	UPROPERTY(Transient)
	int32 PendingCombatActionSlot = INDEX_NONE;

	UPROPERTY(Transient)
	FText CombatTargetingHint;

	UPROPERTY(Transient)
	FIntPoint PendingTerritoryCoord = FIntPoint::ZeroValue;

	UPROPERTY(EditDefaultsOnly, Category="Interaction")
	float InteractSearchRadius = 300.f;

	UPROPERTY(EditDefaultsOnly, Category="Interaction")
	float InteractMaxDistance = 1200.f;

	// Combat targets may be selected before the Page enters the skill's shorter attack range.
	UPROPERTY(EditDefaultsOnly, Category="Combat")
	float CombatTargetMaxDistance = 5000.f;

	UPROPERTY(EditDefaultsOnly, Category="Interaction")
	float InteractForwardDotThreshold = 0.55f;

	UPROPERTY(EditDefaultsOnly, Category="World Block", meta=(ClampMin="100.0"))
	float BlockPlacementMaxDistance = 1200.f;

	// The first click acts immediately; holding repeats only non-combat world interactions.
	UPROPERTY(EditDefaultsOnly, Category="Interaction", meta=(ClampMin="0.05", ClampMax="2.0"))
	float WorldInteractionRepeatInterval = 0.35f;

	float LastWorldInteractionTime = -FLT_MAX;

	// Prevent the remainder of an action-selecting click from becoming a held default interaction.
	bool bSuppressWorldInteractionRepeatUntilPrimaryReleased = false;

	UPROPERTY(Transient)
	bool bFirstPersonUIFocusMode = false;
	
};

