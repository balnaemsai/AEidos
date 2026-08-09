#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "BlockInteractionDefinitionRow.generated.h"

/** A separate row per available block action, so blocks can expose any number of actions in CSV. */
USTRUCT(BlueprintType)
struct FBlockInteractionDefinitionRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName BlockId = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName InteractionId = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(MultiLine=true)) FText Description;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName RequiredToolTag = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName ResultItemId = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1")) int32 ResultQuantity = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1")) int32 IntegrityDamage = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bIsDefault = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bConsumesIntegrity = true;
};

