#include "UI/HUD/DungeonStatusWidget.h"

void UDungeonStatusWidget::SetDungeonStatus(const FDungeonStatusView& InStatus)
{
	CachedStatus = InStatus;
}
