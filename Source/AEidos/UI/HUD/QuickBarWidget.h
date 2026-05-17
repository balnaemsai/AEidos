#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/HUD/HUDViewModels.h"
#include "QuickBarWidget.generated.h"

UCLASS()
class AEIDOS_API UQuickBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="QuickBar")
	void SetSlots(const TArray<FPageQuickSlotView>& InSlots);

	UFUNCTION(BlueprintCallable, Category="QuickBar")
	void SetPopupVisible(bool bInVisible);

	UFUNCTION(BlueprintPure, Category="QuickBar")
	const TArray<FPageQuickSlotView>& GetSlots() const { return CachedSlots; }

	UFUNCTION(BlueprintPure, Category="QuickBar")
	bool IsPopupVisible() const { return bPopupVisible; }

protected:
	UPROPERTY(BlueprintReadOnly, Category="QuickBar")
	TArray<FPageQuickSlotView> CachedSlots;

	UPROPERTY(BlueprintReadOnly, Category="QuickBar")
	bool bPopupVisible = false;
};
