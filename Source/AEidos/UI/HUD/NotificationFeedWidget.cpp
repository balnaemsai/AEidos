#include "UI/HUD/NotificationFeedWidget.h"

void UNotificationFeedWidget::PushMessage(const FText& Message, float LifetimeSeconds)
{
	FNotificationMessageView Entry;
	Entry.Message = Message;
	Entry.RemainingSeconds = LifetimeSeconds;
	Messages.Add(Entry);
}

void UNotificationFeedWidget::TickFeed(float DeltaSeconds)
{
	for (int32 Index = Messages.Num() - 1; Index >= 0; --Index)
	{
		Messages[Index].RemainingSeconds -= DeltaSeconds;
		if (Messages[Index].RemainingSeconds <= 0.f)
		{
			Messages.RemoveAt(Index);
		}
	}
}
