#include "UI/HUD/PageRosterWidget.h"

void UPageRosterWidget::SetPages(const TArray<FPageSummaryView>& InPages)
{
	CachedPages = InPages;
}
