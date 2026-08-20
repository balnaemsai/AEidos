#include "UI/Menus/ScenarioSelectionWidget.h"

#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/TextBlock.h"
#include "Data/Definitions/ScenarioDefinitionRow.h"
#include "Data/GIS_DataRegistry.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Framework/MenuPlayerController.h"
#include "Progression/GIS_ScenarioSession.h"

void UScenarioSelectionWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	ComboBox_Scenarios->OnSelectionChanged.AddUniqueDynamic(this, &UScenarioSelectionWidget::HandleScenarioChanged);
	Button_Start->OnClicked.AddUniqueDynamic(this, &UScenarioSelectionWidget::HandleStartClicked);
	Button_Back->OnClicked.AddUniqueDynamic(this, &UScenarioSelectionWidget::HandleBackClicked);
}

void UScenarioSelectionWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshScenarioOptions();
}

void UScenarioSelectionWidget::RefreshScenarioOptions()
{
	ScenarioIdByOption.Empty();
	SelectedScenarioId = NAME_None;
	ComboBox_Scenarios->ClearOptions();

	UGameInstance* GameInstance = GetGameInstance();
	UGIS_DataRegistry* Registry = GameInstance ? GameInstance->GetSubsystem<UGIS_DataRegistry>() : nullptr;
	UDataTable* Table = Registry && Registry->EnsureReadySync() ? Registry->GetScenarioTable() : nullptr;
	if (!Table)
	{
		if (Text_Status) Text_Status->SetText(FText::FromString(TEXT("Scenario data is unavailable.")));
		Button_Start->SetIsEnabled(false);
		return;
	}

	for (const FName RowName : Table->GetRowNames())
	{
		const FScenarioDefinitionRow* Definition = Registry->GetScenarioDef(RowName);
		if (!Definition)
		{
			continue;
		}

		const FName ScenarioId = Definition->ScenarioId.IsNone() ? RowName : Definition->ScenarioId;
		FString Label = Definition->DisplayName.IsEmpty() ? ScenarioId.ToString() : Definition->DisplayName.ToString();
		if (ScenarioIdByOption.Contains(Label))
		{
			Label = FString::Printf(TEXT("%s [%s]"), *Label, *ScenarioId.ToString());
		}
		ScenarioIdByOption.Add(Label, ScenarioId);
		ComboBox_Scenarios->AddOption(Label);
	}

	if (ScenarioIdByOption.IsEmpty())
	{
		if (Text_Status) Text_Status->SetText(FText::FromString(TEXT("No scenarios are defined.")));
		Button_Start->SetIsEnabled(false);
		return;
	}

	TArray<FString> Labels;
	ScenarioIdByOption.GetKeys(Labels);
	Labels.Sort();
	ComboBox_Scenarios->SetSelectedOption(Labels[0]);
	Button_Start->SetIsEnabled(true);
	if (Text_Status) Text_Status->SetText(FText::GetEmpty());
}

void UScenarioSelectionWidget::HandleScenarioChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (const FName* ScenarioId = ScenarioIdByOption.Find(SelectedItem))
	{
		SelectedScenarioId = *ScenarioId;
	}
	else
	{
		SelectedScenarioId = NAME_None;
	}
	ShowSelectedScenario();
}

void UScenarioSelectionWidget::ShowSelectedScenario()
{
	UGameInstance* GameInstance = GetGameInstance();
	UGIS_DataRegistry* Registry = GameInstance ? GameInstance->GetSubsystem<UGIS_DataRegistry>() : nullptr;
	const FScenarioDefinitionRow* Definition = Registry && Registry->EnsureReadySync() ? Registry->GetScenarioDef(SelectedScenarioId) : nullptr;
	if (!Definition)
	{
		return;
	}

	if (Text_ScenarioName) Text_ScenarioName->SetText(Definition->DisplayName);
	if (Text_ScenarioDescription) Text_ScenarioDescription->SetText(Definition->Description);
}

void UScenarioSelectionWidget::HandleStartClicked()
{
	if (SelectedScenarioId.IsNone())
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UGIS_ScenarioSession* ScenarioSession = GameInstance ? GameInstance->GetSubsystem<UGIS_ScenarioSession>() : nullptr;
	AMenuPlayerController* MenuController = Cast<AMenuPlayerController>(GetOwningPlayer());
	if (!ScenarioSession || !MenuController)
	{
		if (Text_Status) Text_Status->SetText(FText::FromString(TEXT("Unable to start the selected scenario.")));
		return;
	}

	ScenarioSession->SetPendingNewGameScenario(SelectedScenarioId);
	MenuController->RequestNewGame();
}

void UScenarioSelectionWidget::HandleBackClicked()
{
	if (AMenuPlayerController* MenuController = Cast<AMenuPlayerController>(GetOwningPlayer()))
	{
		MenuController->FocusMainMenu();
	}
}
