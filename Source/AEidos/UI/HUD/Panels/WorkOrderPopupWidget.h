#pragma once

#include "CoreMinimal.h"
#include "Core/Types/WorkTypes.h"
#include "Blueprint/UserWidget.h"
#include "WorkOrderPopupWidget.generated.h"
class UButton; class UTextBlock; class UVerticalBox; class UWorkOrderEntry; class UWS_Work;
UCLASS()
class AEIDOS_API UWorkOrderPopupWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	UFUNCTION(BlueprintCallable) void RefreshOrders();
protected:
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UVerticalBox> VerticalBox_WorkOrders;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Empty;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_Close;
	UPROPERTY(EditDefaultsOnly) TSubclassOf<UWorkOrderEntry> WorkOrderEntryClass;
	UFUNCTION() void HandleOrderRequested(FName WorkId);
	UFUNCTION() void HandleAbortOrderRequested(int32 RequestId);
	UFUNCTION() void HandleCloseClicked();
	void HandleWorkRequestStateChanged(int32 RequestId, EWorkRequestLifecycleState NewState);
	void EnableOrderInput();

private:
	TWeakObjectPtr<UWS_Work> ObservedWorkSystem;
	FDelegateHandle WorkRequestStateChangedHandle;
	FTimerHandle EnableOrderInputTimer;
	bool bAcceptOrderInput = false;
};
