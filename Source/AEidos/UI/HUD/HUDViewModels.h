#pragma once

#include "CoreMinimal.h"
#include "Entities/Page/PageCharacter.h"
#include "Core/Types/PortalTypes.h"
#include "HUDViewModels.generated.h"

USTRUCT(BlueprintType)
struct FPageQuickSlotView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly)
	FText SlotLabel;

	UPROPERTY(BlueprintReadOnly)
	EPageCombatActionType ActionType = EPageCombatActionType::None;

	UPROPERTY(BlueprintReadOnly)
	FName ActionId;

	UPROPERTY(BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly)
	bool bAssigned = false;
};

USTRUCT(BlueprintType)
struct FPageActionCandidateView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	EPageCombatActionType ActionType = EPageCombatActionType::None;

	UPROPERTY(BlueprintReadOnly)
	FName ActionId;

	UPROPERTY(BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly)
	int32 ActionPointCost = 0;

	UPROPERTY(BlueprintReadOnly)
	bool bAssignedToQuickBar = false;
};

USTRUCT(BlueprintType)
struct FPageSummaryView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 PageId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly)
	EPageFaction Faction = EPageFaction::Friendly;

	UPROPERTY(BlueprintReadOnly)
	bool bIsSelected = false;

	UPROPERTY(BlueprintReadOnly)
	bool bIsInDungeon = false;

	UPROPERTY(BlueprintReadOnly)
	bool bIsInTurnCombat = false;

	UPROPERTY(BlueprintReadOnly)
	bool bHasActiveCombatTurn = false;

	UPROPERTY(BlueprintReadOnly)
	bool bIsDead = false;

	UPROPERTY(BlueprintReadOnly)
	float Health = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float MaxHealth = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float Hunger = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float Fatigue = 0.f;

	UPROPERTY(BlueprintReadOnly)
	int32 ActionPointsRemaining = 0;

	UPROPERTY(BlueprintReadOnly)
	FText StatusText;

	UPROPERTY(BlueprintReadOnly)
	float CurrentInventoryVolume = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float MaxInventoryVolume = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float CurrentInventoryWeight = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float MaxInventoryWeight = 0.f;
};

USTRUCT(BlueprintType)
struct FDungeonStatusView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	bool bVisible = false;

	UPROPERTY(BlueprintReadOnly)
	FText Title;

	UPROPERTY(BlueprintReadOnly)
	FText Objective;

	UPROPERTY(BlueprintReadOnly)
	FText Secondary;

	UPROPERTY(BlueprintReadOnly)
	int32 ActivePortalCount = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 SelectedPageId = INDEX_NONE;
};

USTRUCT(BlueprintType)
struct FNotificationMessageView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FText Message;

	UPROPERTY(BlueprintReadOnly)
	float RemainingSeconds = 0.f;
};
