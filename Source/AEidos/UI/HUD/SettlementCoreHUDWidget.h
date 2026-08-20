#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SettlementCoreHUDWidget.generated.h"

class UProgressBar;
class UTextBlock;

/** Compact persistent readout for the settlement defeat objective. */
UCLASS()
class AEIDOS_API USettlementCoreHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_CoreLabel;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UProgressBar> ProgressBar_CoreHealth;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_CoreHealth;

	UPROPERTY(EditDefaultsOnly, Category="Settlement|Core", meta=(ClampMin="0.03"))
	float RefreshIntervalSeconds = 0.10f;

private:
	void RefreshCoreReadout();
	float RefreshAccumulator = 0.f;
};
