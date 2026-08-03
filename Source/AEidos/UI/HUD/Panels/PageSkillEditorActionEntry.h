#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/HUD/HUDViewModels.h"
#include "PageSkillEditorActionEntry.generated.h"

class UBorder;
class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPageSkillEditorActionClicked, FPageActionCandidateView, Action);

UCLASS()
class AEIDOS_API UPageSkillEditorActionEntry : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	void Setup(const FPageActionCandidateView& InAction, bool bSelected);

	UPROPERTY(BlueprintAssignable, Category="Pages")
	FOnPageSkillEditorActionClicked OnActionClicked;

protected:
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_Root;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UBorder> Border_Background;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_ActionName;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_APCost;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Assignment;

	UPROPERTY(BlueprintReadOnly, Category="Pages") FPageActionCandidateView ActionData;

	UFUNCTION() void HandleClicked();
};
