#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PanelLifecycle.generated.h"

UINTERFACE(BlueprintType)
class AEIDOS_API UPanelLifecycle : public UInterface
{
	GENERATED_BODY()
};

class AEIDOS_API IPanelLifecycle
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Panel")
	void OnPanelShown();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Panel")
	void OnPanelHidden();
};