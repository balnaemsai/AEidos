#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/HUD/HUDViewModels.h"
#include "DungeonEntry.generated.h"

class UBorder;
class UButton;
class UImage;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDungeonEntryClicked, int32, PortalId);

UCLASS()
class AEIDOS_API UDungeonEntry : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category="Dungeons")
	void Setup(const FDungeonPortalView& InView);

	UPROPERTY(BlueprintAssignable, Category="Dungeons")
	FOnDungeonEntryClicked OnEntryClicked;

protected:
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UBorder> Border_Selection;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UButton> Button_Root;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UImage> Image_Status;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Title;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Subtitle;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Timer;

	UPROPERTY(BlueprintReadOnly, Category="Dungeons")
	FDungeonPortalView ViewData;

	UFUNCTION()
	void HandleClicked();
};
