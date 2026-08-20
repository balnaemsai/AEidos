#include "UI/HUD/ScenarioVictoryWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "UI/GIS_UIRouter.h"

void UScenarioVictoryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (Button_ReturnToMenu)
	{
		Button_ReturnToMenu->OnClicked.AddUniqueDynamic(this, &UScenarioVictoryWidget::HandleReturnToMenu);
	}
}

void UScenarioVictoryWidget::SetScenarioResult(const FText& ScenarioName, const FText& ScenarioDescription)
{
	if (Text_ScenarioName) Text_ScenarioName->SetText(ScenarioName);
	if (Text_ScenarioDescription) Text_ScenarioDescription->SetText(ScenarioDescription);
}

void UScenarioVictoryWidget::HandleReturnToMenu()
{
	if (!GetWorld()) return;
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UGIS_UIRouter* UIRouter = GameInstance->GetSubsystem<UGIS_UIRouter>())
		{
			UIRouter->RequestUIState(EUIState::MainMenu);
		}
	}
	UGameplayStatics::SetGamePaused(this, false);
	UGameplayStatics::OpenLevel(this, MenuMapName);
}
