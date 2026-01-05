#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SaveGameSchema.h"
#include "SaveGameParticipant.generated.h"

UINTERFACE(BlueprintType)
class AEIDOS_API USaveGameParticipant : public UInterface
{
	GENERATED_BODY()
};

class AEIDOS_API ISaveGameParticipant
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Save")
	void WriteToSnapshot(FEidosWorldSnapshot& InOutSnapshot) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Save")
	void ApplySnapshot(const FEidosWorldSnapshot& Snapshot);
};
