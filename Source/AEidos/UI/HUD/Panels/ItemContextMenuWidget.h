#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/Types/ItemTypes.h"
#include "ItemContextMenuWidget.generated.h"

class UVerticalBox;
class UTextBlock;
class UItemContextOptionWidget;

USTRUCT(BlueprintType)
struct FItemContextActionView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) EInventoryItemActionType Action = EInventoryItemActionType::Use;
	UPROPERTY(BlueprintReadOnly) FText Label;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemContextActionSelected, EInventoryItemActionType, Action);

UCLASS()
class AEIDOS_API UItemContextMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ShowMenu(const FText& SelectionSummary, const TArray<FItemContextActionView>& Actions, const FVector2D& ScreenPosition);
	UPROPERTY(BlueprintAssignable, Category="Items") FOnItemContextActionSelected OnActionSelected;

protected:
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_SelectionSummary;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UVerticalBox> VerticalBox_Actions;
	UPROPERTY(EditDefaultsOnly, Category="Items") TSubclassOf<UItemContextOptionWidget> OptionClass;
	UFUNCTION() void HandleOptionClicked(EInventoryItemActionType Action);
};
