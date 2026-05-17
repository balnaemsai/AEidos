#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/HUD/HUDViewModels.h"
#include "DungeonStatusWidget.generated.h"

UCLASS()
class AEIDOS_API UDungeonStatusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Dungeon")
	void SetDungeonStatus(const FDungeonStatusView& InStatus);

	UFUNCTION(BlueprintPure, Category="Dungeon")
	const FDungeonStatusView& GetDungeonStatus() const { return CachedStatus; }

protected:
	UPROPERTY(BlueprintReadOnly, Category="Dungeon")
	FDungeonStatusView CachedStatus;
};
