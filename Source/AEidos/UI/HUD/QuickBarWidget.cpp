#include "UI/HUD/QuickBarWidget.h"

void UQuickBarWidget::SetSlots(const TArray<FPageQuickSlotView>& InSlots)
{
	CachedSlots = InSlots;
}

void UQuickBarWidget::SetPopupVisible(bool bInVisible)
{
	bPopupVisible = bInVisible;
}
