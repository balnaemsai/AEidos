#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "BlockDefinitionRow.generated.h"

/** One CSV row describing a block's shared state. Its visual mesh remains on the block Blueprint. */
USTRUCT(BlueprintType)
struct FBlockDefinitionRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName BlockId = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1")) int32 MaxIntegrity = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bDestroyWhenDepleted = true;
};

