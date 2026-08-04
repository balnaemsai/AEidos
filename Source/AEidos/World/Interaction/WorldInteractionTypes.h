#pragma once

#include "CoreMinimal.h"
#include "WorldInteractionTypes.generated.h"

USTRUCT(BlueprintType)
struct FWorldInteractionOption
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName InteractionId = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Description;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName RequiredToolTag = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bIsDefault = false;
};
