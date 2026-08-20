#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/Types/WorkTypes.h"
#include "PageWorkPriorityRow.generated.h"

class UButton;
class UTextBlock;
class UPageWorkPriorityEditorWidget;

UCLASS()
class AEIDOS_API UPageWorkPriorityRow : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	void Setup(UPageWorkPriorityEditorWidget* InEditor, EWorkCategory InCategory, int32 InPriority);

protected:
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Category;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Priority;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_Decrease;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_Increase;

	UFUNCTION() void HandleDecreaseClicked();
	UFUNCTION() void HandleIncreaseClicked();

private:
	TWeakObjectPtr<UPageWorkPriorityEditorWidget> Editor;
	EWorkCategory WorkCategory = EWorkCategory::Craft;
	int32 Priority = 3;
	void RefreshText();
};
