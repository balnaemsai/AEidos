#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "PortalDefinitionRow.generated.h"

USTRUCT(BlueprintType)
struct FPortalDefinitionRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName PortalId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftClassPtr<AActor> PortalActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float SpawnIntervalSeconds = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float RaidDelaySeconds = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Tier = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName DungeonMapId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float SpawnMinDistance = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float SpawnMaxDistance = 2800.f;
};