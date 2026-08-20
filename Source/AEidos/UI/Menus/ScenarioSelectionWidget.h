#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ScenarioSelectionWidget.generated.h"

class UButton;
class UComboBoxString;
class UTextBlock;

/** Menu-side selector for a data-driven scenario before a fresh settlement is created. */
UCLASS()
class AEIDOS_API UScenarioSelectionWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UComboBoxString> ComboBox_Scenarios;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> Button_Start;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> Button_Back;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ScenarioName;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ScenarioDescription;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Status;

private:
	UFUNCTION()
	void HandleScenarioChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleStartClicked();

	UFUNCTION()
	void HandleBackClicked();

	void RefreshScenarioOptions();
	void ShowSelectedScenario();

	TMap<FString, FName> ScenarioIdByOption;
	FName SelectedScenarioId = NAME_None;
};
