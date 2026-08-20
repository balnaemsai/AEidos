#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ScenarioVictoryWidget.generated.h"

class UButton;
class UTextBlock;

/** Full-screen result modal for a completed scenario. */
UCLASS()
class AEIDOS_API UScenarioVictoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetScenarioResult(const FText& ScenarioName, const FText& ScenarioDescription);

protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void HandleReturnToMenu();

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> Button_ReturnToMenu;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ScenarioName;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ScenarioDescription;

	UPROPERTY(EditDefaultsOnly, Category="Victory")
	FName MenuMapName = TEXT("MenuMap");
};
