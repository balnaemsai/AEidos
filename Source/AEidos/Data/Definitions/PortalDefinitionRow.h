#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "PortalDefinitionRow.generated.h"

class UDungeonSettlementPreset;

USTRUCT(BlueprintType)
struct FPortalDefinitionRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName PortalId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftClassPtr<AActor> PortalActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bEnableAutoSpawn = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float SpawnIntervalSeconds = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0"))
	int32 MaxActiveCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float RaidDelaySeconds = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName DungeonMapId = NAME_None;

	/**
	 * Settlement layout candidates live alongside the portal spawn definition.
	 * This replaces the separate DT_DungeonPreset table.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UDungeonSettlementPreset> PresetAsset;

	/** Difficulty range in which this row can supply a dungeon settlement layout. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.0"))
	float MinimumDifficulty = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.0"))
	float MaximumDifficulty = 999999.f;

	/** Relative chance when several compatible portal rows provide a layout. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.0"))
	float SelectionWeight = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float SpawnMinDistance = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float SpawnMaxDistance = 2800.f;
};
