#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PanelUIFunctionLibrary.generated.h"

class UUserWidget;

/** Blueprint helpers shared by every WBP that is hosted inside WBP_HUDRootWidget. */
UCLASS()
class AEIDOS_API UPanelUIFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Closes the active panel that owns PanelWidget. Returns false outside the in-game HUD tree. */
	UFUNCTION(BlueprintCallable, Category="Panel", meta=(DefaultToSelf="PanelWidget"))
	static bool ClosePanel(UUserWidget* PanelWidget);
};
