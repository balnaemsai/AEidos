// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Entities/Page/PageCharacter.h"
#include "Simulation/SimSystem.h"
#include "Subsystems/WorldSubsystem.h"
#include "WS_CombatDirector.generated.h"

class AActor;
class USimCommandBuffer;
struct FSkillDefinitionRow;

UENUM(BlueprintType)
enum class ECombatEncounterState : uint8
{
	Exploration,
	PlayerTurn,
	EnemyTurn,
	Resolving
};

struct FCombatEncounterRuntime
{
	int32 EncounterId = 0;
	int32 RoundIndex = 1;
	int32 ActiveTurnIndex = 0;
	float TurnTimeRemaining = 0.f;
	bool bCombatSpaceIsDungeon = false;
	ECombatEncounterState State = ECombatEncounterState::Exploration;
	TArray<TWeakObjectPtr<APageCharacter>> Combatants;
};

USTRUCT()
struct FCombatActionPointState
{
	GENERATED_BODY()

	UPROPERTY()
	int32 MaxActionPoints = 2;

	UPROPERTY()
	int32 ActionPointsRemaining = 0;

	UPROPERTY()
	float MovementProgressCm = 0.f;
};

USTRUCT()
struct FCombatInitiativeState
{
	GENERATED_BODY()

	UPROPERTY()
	float InitiativeValue = 0.f;

	UPROPERTY()
	float AgilityValue = 5.f;

	UPROPERTY()
	float ActionThreshold = 10.f;

	UPROPERTY()
	int32 TurnsTaken = 0;
};

USTRUCT()
struct FPendingCombatActionRequest
{
	GENERATED_BODY()

	TWeakObjectPtr<APageCharacter> RequestingPage;
	TWeakObjectPtr<AActor> TargetActor;
	int32 SlotIndex = INDEX_NONE;
	EPageCombatActionType ActionType = EPageCombatActionType::None;
	FName ActionId;
};

/**
 * 
 */
UCLASS()
class AEIDOS_API UWS_CombatDirector : public UWorldSubsystem, public ISimSystem
{
	GENERATED_BODY()

public:
	virtual int32 GetSimOrder_Implementation() const override { return 40; }
	virtual void SimPlan_Implementation(USimCommandBuffer* Cmd, float FixedDeltaSeconds) override;
	virtual void SimCommit_Implementation(USimCommandBuffer* CommandBuffer, float FixedDeltaSeconds) override;
	virtual void SimPost_Implementation(float FixedDeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category="Combat")
	bool IsCombatActive() const;

	UFUNCTION(BlueprintPure, Category="Combat")
	bool IsPageTurnActive(const APageCharacter* Page) const;

	UFUNCTION(BlueprintPure, Category="Combat")
	int32 GetActionPointsRemaining(const APageCharacter* Page) const;

	bool NotifyPageMoved(APageCharacter* Page, float DistanceCm);
	bool RequestEndTurn(APageCharacter* Page);
	bool RequestUseCombatAction(APageCharacter* RequestingPage, int32 SlotIndex, AActor* OptionalTarget);

protected:
	UPROPERTY(EditDefaultsOnly, Category="Combat")
	float DetectionRangeCm = 1200.f;

	UPROPERTY(EditDefaultsOnly, Category="Combat")
	float EncounterStartRangeCm = 400.f;

	UPROPERTY(EditDefaultsOnly, Category="Combat")
	float CombatJoinRangeCm = 500.f;

	UPROPERTY(EditDefaultsOnly, Category="Combat")
	float AttackRangeCm = 150.f;

	UPROPERTY(EditDefaultsOnly, Category="Combat")
	float ChaseSpeedCmPerSecond = 320.f;

	UPROPERTY(EditDefaultsOnly, Category="Combat")
	float MovementPerTurnCm = 280.f;

	UPROPERTY(EditDefaultsOnly, Category="Combat")
	float AttackDamagePerHit = 18.f;

	UPROPERTY(EditDefaultsOnly, Category="Combat")
	float AttackCooldownSeconds = 0.75f;

	UPROPERTY(EditDefaultsOnly, Category="Combat")
	float TurnTimeLimitSeconds = 60.f;

	UPROPERTY(EditDefaultsOnly, Category="Combat")
	float TurnTransitionDelaySeconds = 0.15f;

private:
	TArray<APageCharacter*> GatherLivingPages() const;
	APageCharacter* FindClosestHostileTarget(APageCharacter* Source, const TArray<APageCharacter*>& Candidates, float MaxRangeCm = TNumericLimits<float>::Max()) const;
	TArray<APageCharacter*> GatherEncounterSeedCombatants(APageCharacter* TriggerPage, APageCharacter* TriggerTarget, const TArray<APageCharacter*>& Candidates) const;
	bool TryStartEncounter(const TArray<APageCharacter*>& Pages);
	void StartEncounter(const TArray<APageCharacter*>& Combatants, bool bCombatSpaceIsDungeon);
	void RefreshEncounterState();
	void RebuildCombatantsFromWorld(const TArray<APageCharacter*>& Pages);
	void RecruitNearbyCombatants(const TArray<APageCharacter*>& Pages);
	void UpdateCombatantTurnFlags(APageCharacter* ActivePage);
	void BeginTurnForCombatant(APageCharacter* ActivePage);
	bool HasActionPointsRemaining(APageCharacter* Page) const;
	bool TrySpendActionPoints(APageCharacter* Page, int32 Cost, const TCHAR* Context);
	float GetCombatAgility(const APageCharacter* Page) const;
	float GetActionThreshold(const APageCharacter* Page) const;
	int32 GetActionPointsPerTurn(const APageCharacter* Page) const;
	bool ExecutePendingFriendlyAction(USimCommandBuffer* Cmd, APageCharacter* ActivePage, const TArray<APageCharacter*>& Pages);
	bool ExecuteEnemyTurnStep(USimCommandBuffer* Cmd, APageCharacter* ActivePage, const TArray<APageCharacter*>& Pages);
	void AdvanceTurn();
	void EndEncounter(const TCHAR* Reason);
	void FocusActiveFriendlyPage() const;
	APageCharacter* GetActiveCombatant() const;
	const FSkillDefinitionRow* GetActiveSkillDefinition(FName SkillId) const;

	TMap<TWeakObjectPtr<APageCharacter>, float> Cooldowns;
	TMap<TWeakObjectPtr<APageCharacter>, FCombatActionPointState> CombatantActionPoints;
	TMap<TWeakObjectPtr<APageCharacter>, FCombatInitiativeState> CombatantInitiative;
	FCombatEncounterRuntime ActiveEncounter;
	int32 NextEncounterId = 1;
	bool bAdvanceTurnRequested = false;
	FPendingCombatActionRequest PendingFriendlyAction;
};



