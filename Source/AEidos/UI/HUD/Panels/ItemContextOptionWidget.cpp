#include "UI/HUD/Panels/ItemContextOptionWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

void UItemContextOptionWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (Button_Action && !Button_Action->OnClicked.IsAlreadyBound(this, &UItemContextOptionWidget::HandleClicked))
	{
		Button_Action->OnClicked.AddDynamic(this, &UItemContextOptionWidget::HandleClicked);
	}
}

void UItemContextOptionWidget::Setup(EInventoryItemActionType InAction, const FText& InLabel)
{
	Action = InAction;
	if (Text_Action) Text_Action->SetText(InLabel);
}

void UItemContextOptionWidget::HandleClicked()
{
	OnOptionClicked.Broadcast(Action);
}
