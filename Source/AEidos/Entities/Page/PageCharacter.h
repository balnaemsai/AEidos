// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Entities/Items/EquipmentComponent.h"
#include "Core/Types/WorkTypes.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "PageCharacter.generated.h"

class UStatsComponent;
class UInputMappingContext;
class UInputAction;
class USpringArmComponent;
class UCameraComponent;
class USceneComponent;
class USkillComponent;
class UStaticMeshComponent;
class UInventoryComponent;

UENUM(BlueprintType)
enum class EPageViewMode : uint8
{
	ThirdPerson,
	FirstPerson
};

UENUM(BlueprintType)
enum class EPageFaction : uint8
{
	Friendly,
	Hostile,
	Captive
};

UENUM(BlueprintType)
enum class EPageCombatActionType : uint8
{
	None,
	ActiveSkill,
	ItemUse,
	EndTurn
};

USTRUCT(BlueprintType)
struct FPageCombatActionSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
	EPageCombatActionType ActionType = EPageCombatActionType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
	FName ActionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
	FText DisplayName;
};

USTRUCT(BlueprintType)
struct FPageJobState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 InstanceId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName WorkId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Priority = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector WorkLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsActive = false;

};


UCLASS()
class AEIDOS_API APageCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	APageCharacter();

	UFUNCTION(BlueprintCallable, Category="Page")
	UStatsComponent* GetStats() const { return Stats; }

	void SetViewMode(EPageViewMode NewMode);
	void ToggleViewMode();
	EPageViewMode GetViewMode() const { return ViewMode; }

	UInputMappingContext* GetPageIMC() const { return PageInputMappingContext; }
	USpringArmComponent* GetThirdPersonSpringArm() const { return SpringArm; }
	USceneComponent* GetThirdPersonPivot() const { return ThirdPersonPivot; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Page|Components")
	USkillComponent* Skills;

	UFUNCTION(BlueprintCallable, Category="Skill")
	void AddWorkSkillXP(FName SkillId, float WorkRatePerSecond, float FixedDeltaSeconds, float XPFactor = 1.f);

	// ?대룞 嫄곕━ 湲곕컲 XP (Tick?먯꽌 ?대? ?몄텧)
	UFUNCTION(BlueprintCallable, Category="Skill")
	void AddMovementSkillXP(FName SkillId, float DistanceCm, float XPPerCm);

	// ?꾪닾 ?쒖뒪?쒖뿉???몄텧
	UFUNCTION(BlueprintCallable, Category="Skill")
	void AddCombatSkillXP(FName SkillId, float FlatXP);

	// ?≫떚釉??ㅽ궗 ?쒖뒪?쒖뿉???몄텧
	UFUNCTION(BlueprintCallable, Category="Skill")
	void AddActiveSkillXP(FName SkillId, float FlatXP);

	// ?몃??먯꽌 踰붿슜?곸쑝濡?吏곸젒 二쇨퀬 ?띠쓣 ??
	UFUNCTION(BlueprintCallable, Category="Skill")
	void GainSkillXP(FName SkillId, float Amount, bool bPropagate = true);

	UFUNCTION(BlueprintPure, Category="Skill")
	float GetSkillMultiplier(FName SkillId) const;

	UFUNCTION(BlueprintPure, Category="Skill")
	int32 GetSkillLevel(FName SkillId) const;

	UFUNCTION(BlueprintCallable, Category="Skill")
	void GrantSkill(FName SkillId);

	UFUNCTION(BlueprintPure, Category="Page|Identity")
	int32 GetPageEntityId() const { return PageEntityId; }

	UFUNCTION(BlueprintCallable, Category="Page|Identity")
	void SetPageEntityId(int32 NewPageEntityId);

	UFUNCTION(BlueprintPure, Category="Page|Faction")
	EPageFaction GetFaction() const { return Faction; }

	UFUNCTION(BlueprintCallable, Category="Page|Faction")
	void SetFaction(EPageFaction NewFaction);

	UFUNCTION(BlueprintPure, Category="Page|Faction")
	bool IsFriendly() const { return Faction == EPageFaction::Friendly; }

	UFUNCTION(BlueprintPure, Category="Page|Faction")
	bool IsHostile() const { return Faction == EPageFaction::Hostile; }

	UFUNCTION(BlueprintPure, Category="Page|Faction")
	bool IsCaptive() const { return Faction == EPageFaction::Captive; }

	/** Remaining resistance while this hostile is held captive. Zero permits recruitment. */
	UFUNCTION(BlueprintPure, Category="Page|Captive")
	float GetCaptiveResistance() const { return CaptiveResistance; }

	UFUNCTION(BlueprintCallable, Category="Page|Captive")
	void SetCaptiveResistance(float NewResistance) { CaptiveResistance = FMath::Max(0.f, NewResistance); }

	UFUNCTION(BlueprintCallable, Category="Page|Captive")
	float ReduceCaptiveResistance(float Amount)
	{
		CaptiveResistance = FMath::Max(0.f, CaptiveResistance - FMath::Max(0.f, Amount));
		return CaptiveResistance;
	}

	UFUNCTION(BlueprintPure, Category="Page|Faction")
	bool IsHostileTo(const APageCharacter* OtherPage) const;

	UFUNCTION(BlueprintPure, Category="Page|Dungeon")
	bool IsInDungeon() const { return bIsInDungeon; }

	UFUNCTION(BlueprintCallable, Category="Page|Dungeon")
	void SetIsInDungeon(bool bNewIsInDungeon);

	UFUNCTION(BlueprintPure, Category="Page|Combat")
	bool IsInTurnCombat() const { return bInTurnCombat; }

	UFUNCTION(BlueprintPure, Category="Page|Combat")
	bool HasActiveCombatTurn() const { return bHasActiveCombatTurn; }

	UFUNCTION(BlueprintCallable, Category="Page|Combat")
	void SetTurnCombatState(bool bNewInTurnCombat, bool bNewHasActiveCombatTurn);

	UFUNCTION(BlueprintPure, Category="Page|Combat")
	const TArray<FPageCombatActionSlot>& GetCombatActionSlots() const { return CombatActionSlots; }

	UFUNCTION(BlueprintPure, Category="Page|Combat")
	bool GetCombatActionSlot(int32 SlotIndex, FPageCombatActionSlot& OutSlot) const;

	UFUNCTION(BlueprintCallable, Category="Page|Combat")
	void SetCombatActionSlot(int32 SlotIndex, const FPageCombatActionSlot& InSlot);

	UFUNCTION(BlueprintPure, Category="Page|Inventory")
	float GetCurrentInventoryVolume() const;

	UFUNCTION(BlueprintPure, Category="Page|Inventory")
	float GetMaxInventoryVolume() const;

	UFUNCTION(BlueprintPure, Category="Page|Inventory")
	float GetCurrentInventoryWeight() const;

	UFUNCTION(BlueprintPure, Category="Page|Inventory")
	float GetMaxInventoryWeight() const;

	UFUNCTION(BlueprintPure, Category="Page|Inventory")
	UInventoryComponent* GetInventory() const { return Inventory; }

	UFUNCTION(BlueprintPure, Category="Page|Equipment")
	UEquipmentComponent* GetEquipment() const { return Equipment; }

	UFUNCTION(BlueprintPure, Category="Page|Inventory")
	float GetOverloadRatio() const;

	UFUNCTION(BlueprintPure, Category="Page|Inventory")
	float GetOverloadMovementMultiplier() const;

	UFUNCTION(BlueprintCallable, Category="Page|Inventory")
	void SetInventorySummary(float InCurrentVolume, float InMaxVolume, float InCurrentWeight, float InMaxWeight);

	UFUNCTION(BlueprintCallable, Category="Page|Settlement")
	void SetSettlementOverCapacity(bool bNewOverCapacity);

	UFUNCTION(BlueprintPure, Category="Page|Settlement")
	bool IsSettlementOverCapacity() const { return bSettlementOverCapacity; }

	/** Starts or clears the settlement-wide starvation state for this friendly Page. */
	UFUNCTION(BlueprintCallable, Category="Page|Settlement|Sustenance")
	void SetSettlementFoodShortage(bool bNewFoodShortage);

	/** Advances starvation only while the settlement has failed its meal service. */
	void AdvanceSettlementStarvation(float DeltaSeconds);

	UFUNCTION(BlueprintPure, Category="Page|Settlement|Sustenance")
	bool HasSettlementFoodShortage() const { return bSettlementFoodShortage; }

	UFUNCTION(BlueprintPure, Category="Page|Settlement|Sustenance")
	float GetSettlementStarvationSeverity() const { return SettlementStarvationSeverity; }

	float GetSettlementMovementMultiplier() const;
	float GetSettlementWorkRateMultiplier() const;
	float GetSettlementCombatDamageMultiplier() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Page")
	FPageJobState CurrentJobState;

	/**
	 * Marks this Page as player-directed for a short period.  Automatic work
	 * assignment must wait until player input has stopped before taking control back.
	 */
	UFUNCTION(BlueprintCallable, Category="Page|Work")
	void BeginManualWorkOverride();

	UFUNCTION(BlueprintPure, Category="Page|Work")
	bool IsManualWorkOverrideActive() const;

	/** Returns this Page's category priority. Missing entries use the neutral default of 3. */
	UFUNCTION(BlueprintPure, Category="Page|Work")
	int32 GetWorkPriority(EWorkCategory WorkCategory) const;

	UFUNCTION(BlueprintCallable, Category="Page|Work")
	void SetWorkPriority(EWorkCategory WorkCategory, int32 NewPriority);

	const TArray<FPageWorkPriority>& GetWorkPriorities() const { return WorkPriorities; }
	void SetWorkPriorities(const TArray<FPageWorkPriority>& InPriorities);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	void HandleMove(const FInputActionValue& Value);
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	UStatsComponent* Stats;
	
	UPROPERTY(EditDefaultsOnly, Category="Input")
	UInputMappingContext* PageInputMappingContext;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	UInputAction* MoveAction;

	/** Seconds after the last manual input before automatic work may resume. */
	UPROPERTY(EditDefaultsOnly, Category="Page|Work", meta=(ClampMin="0.0", ClampMax="10.0"))
	float ManualWorkOverrideGraceSeconds = 1.25f;

	UPROPERTY(Transient)
	float ManualWorkOverrideUntilTime = -1.f;

	/** Per-category preferences used by automatic work assignment. Empty entries mean priority 3. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Page|Work")
	TArray<FPageWorkPriority> WorkPriorities;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
	USpringArmComponent* SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
	UCameraComponent* ThirdPersonCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
	UCameraComponent* FirstPersonCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Page|Combat", meta=(AllowPrivateAccess="true"))
	UStaticMeshComponent* CombatIndicatorMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Page|Inventory", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UInventoryComponent> Inventory;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Page|Equipment", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UEquipmentComponent> Equipment;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
	EPageViewMode ViewMode = EPageViewMode::ThirdPerson;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera", meta=(AllowPrivateAccess="true"))
	USceneComponent* ThirdPersonPivot;

	void TickMovementSkillGain(float DeltaSeconds);
	void EnsureDefaultCombatLoadout();

	// ?ㅽ궗 ?뺤쓽 湲곕낯 ?명똿
	void BuildDefaultSkillDefinitions();
	void BuildDefaultWorkSkillMap();

	// ?대? ?좏떥
	void EnsureSkillStateExists(FName SkillId);
	void GrantSkillExp_Internal(FName SkillId, float ExpAmount, bool bAllowRelatedPropagation);
	float GetTotalExpRequiredToReachLevel(FName SkillId, int32 TargetLevel) const;
	int32 EvaluateLevelFromTotalExp(FName SkillId, float InTotalExp) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Page|Runtime")
	FVector PreviousWorldLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Page|Identity")
	int32 PageEntityId = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Page|Dungeon")
	bool bIsInDungeon = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Page|Combat")
	bool bInTurnCombat = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Page|Combat")
	bool bHasActiveCombatTurn = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Page|Faction")
	EPageFaction Faction = EPageFaction::Friendly;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Page|Captive", meta=(ClampMin="0.0"))
	float CaptiveResistance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Page|Combat")
	TArray<FPageCombatActionSlot> CombatActionSlots;

	// Blueprint presets define which skills this Page starts with. Slots may reference only owned skills.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Page|Combat")
	TArray<FName> DefaultSkillIds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Page|Inventory")
	float CurrentInventoryVolume = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Page|Inventory")
	float MaxInventoryVolume = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Page|Inventory")
	float CurrentInventoryWeight = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Page|Inventory")
	float MaxInventoryWeight = 50.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Page|Inventory|Overload", meta=(ClampMin="0.0"))
	float BaseWalkSpeed = 450.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Page|Inventory|Overload", meta=(ClampMin="0.01"))
	float OverloadDecayRate = 1.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Page|Inventory|Overload", meta=(ClampMin="0.01", ClampMax="1.0"))
	float MinimumOverloadSpeedMultiplier = 0.10f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Page|Settlement")
	bool bSettlementOverCapacity = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Page|Settlement", meta=(ClampMin="0.01", ClampMax="1.0"))
	float OverCapacityMovementMultiplier = 0.75f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Page|Settlement|Sustenance")
	bool bSettlementFoodShortage = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Page|Settlement|Sustenance")
	float SettlementStarvationSeverity = 0.f;

	/** Seconds of an uninterrupted food shortage before the full starvation penalty applies. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Page|Settlement|Sustenance", meta=(ClampMin="1.0"))
	float StarvationSecondsToMaximum = 600.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Page|Settlement|Sustenance", meta=(ClampMin="0.01", ClampMax="1.0"))
	float StarvationMinimumMovementMultiplier = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Page|Settlement|Sustenance", meta=(ClampMin="0.01", ClampMax="1.0"))
	float StarvationMinimumWorkRateMultiplier = 0.40f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Page|Settlement|Sustenance", meta=(ClampMin="0.01", ClampMax="1.0"))
	float StarvationMinimumCombatDamageMultiplier = 0.45f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Page|Settlement|Sustenance", meta=(ClampMin="0.001", ClampMax="1.0"))
	float StarvationMinimumHealthFraction = 0.01f;

	UFUNCTION()
	void HandleInventoryChanged();

	void UpdateOverloadMovementSpeed();

	// cm ??Running 寃쏀뿕移?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Page|Skills|Tuning")
	float RunningExpPerCm = 0.0025f;

	// ?덈Т ?먮┛ ?대룞? ?щ━湲곕줈 ??移쒕떎
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Page|Skills|Tuning")
	float MinimumSpeedForRunningXP = 120.f;
	
};

