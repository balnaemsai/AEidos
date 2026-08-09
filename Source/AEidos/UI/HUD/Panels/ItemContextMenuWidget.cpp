#include "UI/HUD/Panels/ItemContextMenuWidget.h"

#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "UI/HUD/Panels/ItemContextOptionWidget.h"

void UItemContextMenuWidget::ShowMenu(const FText& SelectionSummary, const TArray<FItemContextActionView>& Actions,
	const FVector2D& ScreenPosition)
{
	if (!OptionClass)
	{
		OptionClass = LoadClass<UItemContextOptionWidget>(nullptr, TEXT("/Game/Blueprints/WBP/WBP_ItemContextOption.WBP_ItemContextOption_C"));
	}
	if (Text_SelectionSummary) Text_SelectionSummary->SetText(SelectionSummary);
	if (VerticalBox_Actions)
	{
		VerticalBox_Actions->ClearChildren();
		for (const FItemContextActionView& Action : Actions)
		{
			if (!OptionClass) continue;
			if (UItemContextOptionWidget* Option = CreateWidget<UItemContextOptionWidget>(this, OptionClass))
			{
				Option->Setup(Action.Action, Action.Label);
				Option->OnOptionClicked.AddDynamic(this, &UItemContextMenuWidget::HandleOptionClicked);
				VerticalBox_Actions->AddChildToVerticalBox(Option);
			}
		}
	}
	SetPositionInViewport(ScreenPosition, true);
}

void UItemContextMenuWidget::HandleOptionClicked(EInventoryItemActionType Action)
{
	OnActionSelected.Broadcast(Action);
}
