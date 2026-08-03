#include "UI/HUD/CombatQuickbarSlot.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UCombatQuickbarSlot::NativeConstruct()
{
	Super::NativeConstruct();
	if (Button_Action)
	{
		Button_Action->OnClicked.AddDynamic(this, &UCombatQuickbarSlot::HandleClicked);
	}
}

void UCombatQuickbarSlot::Setup(const FPageQuickSlotView& InView)
{
	ViewData = InView;
	if (Text_Key) Text_Key->SetText(ViewData.SlotLabel);
	// Empty action cells keep only their key number; a large placeholder label
	// makes compact combat bars unreadable and competes with assigned skills.
	if (Text_ActionName) Text_ActionName->SetText(ViewData.bAssigned ? ViewData.DisplayName : FText::GetEmpty());
	if (Text_APCost) Text_APCost->SetText(ViewData.bAssigned ? FText::Format(FText::FromString(TEXT("AP {0}")), ViewData.ActionPointCost) : FText::GetEmpty());
	if (Button_Action) Button_Action->SetIsEnabled(ViewData.bCanUse);
	if (Image_DisabledMark) Image_DisabledMark->SetVisibility(ViewData.bCanUse ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);

	if (Border_SelectionFrame)
	{
		Border_SelectionFrame->SetBrushColor(ViewData.bIsSelected
			? FLinearColor(0.79f, 0.76f, 0.68f, 0.75f)
			: (ViewData.bCanUse ? FLinearColor(0.35f, 0.38f, 0.39f, 0.28f) : FLinearColor(0.35f, 0.38f, 0.39f, 0.12f)));
	}
	if (Border_Surface)
	{
		Border_Surface->SetBrushColor(ViewData.bCanUse
			? FLinearColor(0.07f, 0.085f, 0.095f, 0.90f)
			: FLinearColor(0.035f, 0.04f, 0.045f, 0.68f));
	}

	const FText NewTooltipText = ViewData.bCanUse ? ViewData.DisplayName : ViewData.DisabledReason;
	if (!CachedTooltipText.EqualTo(NewTooltipText))
	{
		CachedTooltipText = NewTooltipText;
		SetToolTipText(NewTooltipText);
	}
}

void UCombatQuickbarSlot::HandleClicked()
{
	if (ViewData.bCanUse)
	{
		OnSlotClicked.Broadcast(ViewData.SlotIndex);
	}
}
