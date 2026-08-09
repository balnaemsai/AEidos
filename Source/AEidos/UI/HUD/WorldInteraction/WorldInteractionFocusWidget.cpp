#include "UI/HUD/WorldInteraction/WorldInteractionFocusWidget.h"

#include "Components/TextBlock.h"

void UWorldInteractionFocusWidget::ShowFocus(const FText& BlockName, const FText& PreparedInteraction)
{
	if (Text_BlockName) Text_BlockName->SetText(BlockName);
	if (Text_PreparedInteraction) Text_PreparedInteraction->SetText(PreparedInteraction);
}
