#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WorldInteractionFocusWidget.generated.h"

class UTextBlock;

/** Compact world-space-style HUD label for the block currently under the player's focus. */
UCLASS()
class AEIDOS_API UWorldInteractionFocusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ShowFocus(const FText& BlockName, const FText& PreparedInteraction);

protected:
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_BlockName;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_PreparedInteraction;
};
