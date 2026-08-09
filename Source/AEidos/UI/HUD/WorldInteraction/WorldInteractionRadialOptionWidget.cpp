#include "UI/HUD/WorldInteraction/WorldInteractionRadialOptionWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Framework/EidosPlayerController.h"

void UWorldInteractionRadialOptionWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (Button_Action && !Button_Action->OnClicked.IsAlreadyBound(this, &UWorldInteractionRadialOptionWidget::HandleActionClicked))
	{
		Button_Action->OnClicked.AddDynamic(this, &UWorldInteractionRadialOptionWidget::HandleActionClicked);
	}
}

void UWorldInteractionRadialOptionWidget::SetupOption(AEidosPlayerController* InController, const FWorldInteractionOption& InOption)
{
	OwningController = InController;
	InteractionId = InOption.InteractionId;
	if (Text_ActionName) Text_ActionName->SetText(InOption.DisplayName);
	if (Text_Requirement)
	{
		const bool bHasRequirement = !InOption.RequiredToolTag.IsNone();
		Text_Requirement->SetVisibility(bHasRequirement ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		if (bHasRequirement)
		{
			Text_Requirement->SetText(FText::Format(FText::FromString(TEXT("Requires {0}")), FText::FromName(InOption.RequiredToolTag)));
		}
	}
}

void UWorldInteractionRadialOptionWidget::HandleActionClicked()
{
	if (AEidosPlayerController* Controller = OwningController.Get())
	{
		Controller->SelectContextWorldInteraction(InteractionId);
	}
}
