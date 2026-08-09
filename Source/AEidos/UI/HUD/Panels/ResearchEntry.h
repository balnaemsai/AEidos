#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "World/Settlement/WS_Research.h"
#include "ResearchEntry.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnResearchStartRequested, FName, ResearchId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnResearchCancelRequested, FName, ResearchId);

UCLASS()
class AEIDOS_API UResearchEntry : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	void Setup(const FResearchView& InView);
	FName GetResearchId() const { return ResearchId; }
	UPROPERTY(BlueprintAssignable) FOnResearchStartRequested OnResearchStartRequested;
	UPROPERTY(BlueprintAssignable) FOnResearchCancelRequested OnResearchCancelRequested;

protected:
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_Start;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_Cancel;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Name;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Description;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Prerequisites;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Status;
	UFUNCTION() void HandleStartClicked();
	UFUNCTION() void HandleCancelClicked();

private:
	FName ResearchId = NAME_None;
};
