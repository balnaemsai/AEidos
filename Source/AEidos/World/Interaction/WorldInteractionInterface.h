#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "World/Interaction/WorldInteractionTypes.h"
#include "WorldInteractionInterface.generated.h"

class APageCharacter;

UINTERFACE(BlueprintType)
class AEIDOS_API UWorldInteractionInterface : public UInterface
{
	GENERATED_BODY()
};

class AEIDOS_API IWorldInteractionInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="World Interaction")
	void GetAvailableWorldInteractions(APageCharacter* InteractingPage, TArray<FWorldInteractionOption>& OutOptions);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="World Interaction")
	bool ExecuteWorldInteraction(APageCharacter* InteractingPage, FName InteractionId);
};
