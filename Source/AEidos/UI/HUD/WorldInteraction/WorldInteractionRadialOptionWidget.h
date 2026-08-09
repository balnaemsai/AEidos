#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "World/Interaction/WorldInteractionTypes.h"
#include "WorldInteractionRadialOptionWidget.generated.h"

class AEidosPlayerController;
class UButton;
class UTextBlock;

UCLASS()
class AEIDOS_API UWorldInteractionRadialOptionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	void SetupOption(AEidosPlayerController* InController, const FWorldInteractionOption& InOption);

protected:
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_Action;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_ActionName;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Requirement;

	UFUNCTION() void HandleActionClicked();

private:
	TWeakObjectPtr<AEidosPlayerController> OwningController;
	FName InteractionId = NAME_None;
};
