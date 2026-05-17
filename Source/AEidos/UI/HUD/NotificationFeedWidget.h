#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/HUD/HUDViewModels.h"
#include "NotificationFeedWidget.generated.h"

UCLASS()
class AEIDOS_API UNotificationFeedWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Notifications")
	void PushMessage(const FText& Message, float LifetimeSeconds = 3.f);

	UFUNCTION(BlueprintCallable, Category="Notifications")
	void TickFeed(float DeltaSeconds);

	UFUNCTION(BlueprintPure, Category="Notifications")
	const TArray<FNotificationMessageView>& GetMessages() const { return Messages; }

protected:
	UPROPERTY(BlueprintReadOnly, Category="Notifications")
	TArray<FNotificationMessageView> Messages;
};
