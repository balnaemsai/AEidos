#include "UI/HUD/Panels/PageEquipmentItemEntry.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void UPageEquipmentItemEntry::NativeConstruct()
{
	Super::NativeConstruct();
	if (Button_Root && !Button_Root->OnClicked.IsAlreadyBound(this, &UPageEquipmentItemEntry::HandleClicked))
	{
		Button_Root->OnClicked.AddDynamic(this, &UPageEquipmentItemEntry::HandleClicked);
	}
}

void UPageEquipmentItemEntry::Setup(FName InItemId, const FText& InDisplayName, int32 InQuantity, const FText& InCompatibleSlots, bool bSelected)
{
	ItemId = InItemId;
	if (Text_ItemName) Text_ItemName->SetText(InDisplayName);
	if (Text_Quantity) Text_Quantity->SetText(FText::Format(FText::FromString(TEXT("x{0}")), InQuantity));
	if (Text_CompatibleSlots) Text_CompatibleSlots->SetText(InCompatibleSlots);
	if (Border_Background)
	{
		Border_Background->SetBrushColor(bSelected
			? FLinearColor(0.16f, 0.18f, 0.19f, 0.96f)
			: FLinearColor(0.06f, 0.07f, 0.08f, 0.88f));
	}
}

void UPageEquipmentItemEntry::HandleClicked()
{
	OnItemClicked.Broadcast(ItemId);
}
