#include "UI/HUD/SettlementDefeatWidget.h"

#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "UI/GIS_UIRouter.h"

void USettlementDefeatWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_ReturnToMenu)
	{
		Button_ReturnToMenu->OnClicked.AddUniqueDynamic(this, &USettlementDefeatWidget::HandleReturnToMenu);
	}
}

void USettlementDefeatWidget::HandleReturnToMenu()
{
	if (!GetWorld())
	{
		return;
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UGIS_UIRouter* UIRouter = GameInstance->GetSubsystem<UGIS_UIRouter>())
		{
			// The router lives across map loads. Leave InGame before opening MenuMap
			// so an AEidosHUD instance cannot recreate the settlement HUD there.
			UIRouter->RequestUIState(EUIState::MainMenu);
		}
	}

	UGameplayStatics::SetGamePaused(this, false);
	UGameplayStatics::OpenLevel(this, MenuMapName);
}
