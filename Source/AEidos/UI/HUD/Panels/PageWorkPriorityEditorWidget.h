#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/Types/WorkTypes.h"
#include "PageWorkPriorityEditorWidget.generated.h"

class APageCharacter;
class UButton;
class UTextBlock;
class UVerticalBox;
class UPageWorkPriorityRow;

UCLASS()
class AEIDOS_API UPageWorkPriorityEditorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	UFUNCTION(BlueprintCallable, Category="Pages|Work") void OpenForPage(APageCharacter* InPage);
	UFUNCTION(BlueprintCallable, Category="Pages|Work") void CloseEditor();
	void SetPriority(EWorkCategory WorkCategory, int32 NewPriority);

protected:
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_PageName;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Instructions;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UVerticalBox> VerticalBox_PriorityRows;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_Close;
	UPROPERTY(EditDefaultsOnly, Category="Pages|Work") TSubclassOf<UPageWorkPriorityRow> PriorityRowClass;

	UFUNCTION() void HandleCloseClicked();

private:
	TWeakObjectPtr<APageCharacter> EditedPage;
	void RefreshEditor();
};
