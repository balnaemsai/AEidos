#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SettlementDefeatWidget.generated.h"

class UButton;

/** Full-screen modal presented when the settlement core is destroyed. */
UCLASS()
class AEIDOS_API USettlementDefeatWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void HandleReturnToMenu();

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> Button_ReturnToMenu;

	/** Keep this configurable in case a scenario uses a different menu map later. */
	UPROPERTY(EditDefaultsOnly, Category="Defeat")
	FName MenuMapName = TEXT("MenuMap");
};
