#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/Types/ItemTypes.h"
#include "ItemContextOptionWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemContextOptionClicked, EInventoryItemActionType, Action);

UCLASS()
class AEIDOS_API UItemContextOptionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	void Setup(EInventoryItemActionType InAction, const FText& InLabel);

	UPROPERTY(BlueprintAssignable, Category="Items") FOnItemContextOptionClicked OnOptionClicked;

protected:
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_Action;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Action;
	UFUNCTION() void HandleClicked();

private:
	EInventoryItemActionType Action = EInventoryItemActionType::Use;
};
