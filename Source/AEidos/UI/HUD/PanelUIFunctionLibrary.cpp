#include "UI/HUD/PanelUIFunctionLibrary.h"

#include "Blueprint/UserWidget.h"
#include "UI/HUD/HUDRootWidget.h"

bool UPanelUIFunctionLibrary::ClosePanel(UUserWidget* PanelWidget)
{
	if (!PanelWidget)
	{
		return false;
	}

	if (UHUDRootWidget* HUDRoot = PanelWidget->GetTypedOuter<UHUDRootWidget>())
	{
		return HUDRoot->CloseActivePanel();
	}

	return false;
}
