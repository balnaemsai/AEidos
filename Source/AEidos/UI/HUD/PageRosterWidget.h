#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/HUD/HUDViewModels.h"
#include "PageRosterWidget.generated.h"

UCLASS()
class AEIDOS_API UPageRosterWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="PageRoster")
	void SetPages(const TArray<FPageSummaryView>& InPages);

	UFUNCTION(BlueprintPure, Category="PageRoster")
	const TArray<FPageSummaryView>& GetPages() const { return CachedPages; }

protected:
	UPROPERTY(BlueprintReadOnly, Category="PageRoster")
	TArray<FPageSummaryView> CachedPages;
};
