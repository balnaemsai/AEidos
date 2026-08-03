#include "UI/HUD/Panels/PageQuickbarSlot.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void UPageQuickbarSlot::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Root)
	{
		Button_Root->OnClicked.AddDynamic(this, &UPageQuickbarSlot::HandleClicked);
	}
}

void UPageQuickbarSlot::Setup(const FPageQuickSlotView& InView)
{
	ViewData = InView;

	RefreshPresentation();
}

void UPageQuickbarSlot::SetEditorSelected(bool bInEditorSelected)
{
	bEditorSelected = bInEditorSelected;
	RefreshPresentation();
}

void UPageQuickbarSlot::RefreshPresentation()
{
	if (Text_KeyNumber) Text_KeyNumber->SetText(ViewData.SlotLabel);
	if (Text_ActionName) Text_ActionName->SetText(ViewData.bAssigned ? ViewData.DisplayName : FText::GetEmpty());
	if (Border_Background)
	{
		Border_Background->SetBrushColor(bEditorSelected
			? FLinearColor(0.26f, 0.25f, 0.22f, 0.95f)
			: (ViewData.bAssigned ? FLinearColor(0.13f, 0.15f, 0.16f, 0.96f) : FLinearColor(0.07f, 0.08f, 0.09f, 0.72f)));
	}
}

void UPageQuickbarSlot::HandleClicked()
{
	OnSlotClicked.Broadcast(ViewData.SlotIndex);
}
