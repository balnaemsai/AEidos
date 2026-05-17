// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
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
	Hostile
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

	// 이동 거리 기반 XP (Tick에서 내부 호출)
	UFUNCTION(BlueprintCallable, Category="Skill")
	void AddMovementSkillXP(FName SkillId, float DistanceCm, float XPPerCm);

	// 전투 시스템에서 호출
	UFUNCTION(BlueprintCallable, Category="Skill")
	void AddCombatSkillXP(FName SkillId, float FlatXP);

	// 액티브 스킬 시스템에서 호출
	UFUNCTION(BlueprintCallable, Category="Skill")
	void AddActiveSkillXP(FName SkillId, float FlatXP);

	// 외부에서 범용적으로 직접 주고 싶을 때
	UFUNCTION(BlueprintCallable, Category="Skill")
	void GainSkillXP(FName SkillId, float Amount, bool bPropagate = true);

	UFUNCTION(BlueprintPure, Category="Skill")
	float GetSkillMultiplier(FName SkillId) const;

	UFUNCTION(BlueprintPure, Category="Skill")
	int32 GetSkillLevel(FName SkillId) const;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Page")
	FPageJobState CurrentJobState;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
	USpringArmComponent* SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
	UCameraComponent* ThirdPersonCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
	UCameraComponent* FirstPersonCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
	EPageViewMode ViewMode = EPageViewMode::ThirdPerson;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera", meta=(AllowPrivateAccess="true"))
	USceneComponent* ThirdPersonPivot;

	void TickMovementSkillGain(float DeltaSeconds);

	// 스킬 정의 기본 세팅
	void BuildDefaultSkillDefinitions();
	void BuildDefaultWorkSkillMap();

	// 내부 유틸
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Page|Combat")
	TArray<FPageCombatActionSlot> CombatActionSlots;

	// cm 당 Running 경험치
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Page|Skills|Tuning")
	float RunningExpPerCm = 0.0025f;

	// 너무 느린 이동은 달리기로 안 친다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Page|Skills|Tuning")
	float MinimumSpeedForRunningXP = 120.f;
	
};
