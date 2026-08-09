#include "UI/HUD/Panels/ItemTransferEntry.h"

#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Input/Events.h"

void UItemTransferEntry::NativeConstruct()
{
	Super::NativeConstruct();
}

FReply UItemTransferEntry::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		OnEntrySelected.Broadcast(ItemId, bFromStorage, InMouseEvent.IsControlDown());
		return FReply::Handled();
	}
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		OnEntryContextRequested.Broadcast(ItemId, bFromStorage);
		return FReply::Handled();
	}
	return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
}

void UItemTransferEntry::Setup(FName InItemId, const FText& InDisplayName, int32 InQuantity, bool bInFromStorage, bool bSelected)
{
	ItemId = InItemId;
	bFromStorage = bInFromStorage;
	if (Text_ItemName) Text_ItemName->SetText(InDisplayName);
	if (Text_Quantity) Text_Quantity->SetText(FText::Format(FText::FromString(TEXT("x{0}")), InQuantity));
	if (Border_Background)
	{
		Border_Background->SetBrushColor(bSelected
			? FLinearColor(0.16f, 0.18f, 0.19f, 0.96f)
			: FLinearColor(0.06f, 0.07f, 0.08f, 0.88f));
	}
}
