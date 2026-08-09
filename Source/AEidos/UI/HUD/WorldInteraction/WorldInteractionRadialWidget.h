#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "World/Interaction/WorldInteractionTypes.h"
#include "WorldInteractionRadialWidget.generated.h"

class AActor;
class AEidosPlayerController;
class UButton;
class UCanvasPanel;
class UTextBlock;
class UWidget;
class UWorldInteractionRadialOptionWidget;

UCLASS()
class AEIDOS_API UWorldInteractionRadialWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	void ShowRadial(AEidosPlayerController* InController, AActor* TargetActor, const TArray<FWorldInteractionOption>& Options, FVector2D ScreenPosition);

protected:
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UCanvasPanel> Canvas_Root;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_Backdrop;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UWidget> Widget_Center;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_TargetName;

	UPROPERTY(EditDefaultsOnly, Category="World Interaction")
	TSubclassOf<UWorldInteractionRadialOptionWidget> OptionWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="World Interaction", meta=(ClampMin="40.0"))
	float OptionRadius = 116.f;

	UPROPERTY(EditDefaultsOnly, Category="World Interaction", meta=(ClampMin="1.0"))
	float EdgePadding = 92.f;

	// Canvas children need explicit sizes at runtime; SizeBox overrides alone are not reliable for dynamically added slots.
	UPROPERTY(EditDefaultsOnly, Category="World Interaction")
	FVector2D CenterWidgetSize = FVector2D(160.f, 64.f);

	UPROPERTY(EditDefaultsOnly, Category="World Interaction")
	FVector2D OptionWidgetSize = FVector2D(146.f, 56.f);

	UFUNCTION() void HandleBackdropClicked();

private:
	void ClearOptionWidgets();
	FVector2D ClampMenuCenter(FVector2D DesiredCenter) const;
	TWeakObjectPtr<AEidosPlayerController> OwningController;
	TArray<TObjectPtr<UWorldInteractionRadialOptionWidget>> SpawnedOptions;
};
