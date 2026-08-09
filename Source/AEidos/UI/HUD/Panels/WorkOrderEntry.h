#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/Types/WorkTypes.h"
#include "WorkOrderEntry.generated.h"

class UButton;
class UTextBlock;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorkOrderRequested, FName, WorkId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorkOrderAbortRequested, int32, RequestId);

UCLASS()
class AEIDOS_API UWorkOrderEntry : public UUserWidget
{
	GENERATED_BODY()
public:
	virtual void NativeConstruct() override;
	void Setup(const FWorkOrderView& InView);
	UPROPERTY(BlueprintAssignable) FOnWorkOrderRequested OnWorkOrderRequested;
	UPROPERTY(BlueprintAssignable) FOnWorkOrderAbortRequested OnWorkOrderAbortRequested;
protected:
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_Order;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_AbortOrder;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Name;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Cost;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Output;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Status;
	UFUNCTION() void HandleOrderPressed();
	UFUNCTION() void HandleOrderClicked();
	UFUNCTION() void HandleAbortOrderClicked();
private:
	FName WorkId;
	int32 CancelRequestId = INDEX_NONE;
	bool bOrderPressStartedOnThisEntry = false;
};
