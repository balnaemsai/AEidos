#include "UI/HUD/WorldInteraction/WorldInteractionRadialWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Framework/EidosPlayerController.h"
#include "UI/HUD/WorldInteraction/WorldInteractionRadialOptionWidget.h"
#include "World/Interaction/WorldItemBlockActor.h"

void UWorldInteractionRadialWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (Button_Backdrop && !Button_Backdrop->OnClicked.IsAlreadyBound(this, &UWorldInteractionRadialWidget::HandleBackdropClicked))
	{
		Button_Backdrop->OnClicked.AddDynamic(this, &UWorldInteractionRadialWidget::HandleBackdropClicked);
	}
}

void UWorldInteractionRadialWidget::ShowRadial(AEidosPlayerController* InController, AActor* TargetActor,
	const TArray<FWorldInteractionOption>& Options, FVector2D ScreenPosition)
{
	OwningController = InController;
	if (!Canvas_Root)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WorldInteraction] Radial WBP is missing Canvas_Root"));
		return;
	}

	ClearOptionWidgets();
	const FVector2D Center = ClampMenuCenter(ScreenPosition);
	if (Text_TargetName)
	{
		FText TargetDisplayName = FText::FromString(GetNameSafe(TargetActor));
		if (const AWorldItemBlockActor* ItemBlock = Cast<AWorldItemBlockActor>(TargetActor); ItemBlock && !ItemBlock->GetItemId().IsNone())
		{
			TargetDisplayName = FText::FromName(ItemBlock->GetItemId());
		}
		Text_TargetName->SetText(TargetDisplayName);
	}
	if (Widget_Center)
	{
		if (UCanvasPanelSlot* CenterSlot = Cast<UCanvasPanelSlot>(Widget_Center->Slot))
		{
			CenterSlot->SetAnchors(FAnchors(0.f, 0.f));
			CenterSlot->SetAutoSize(false);
			CenterSlot->SetSize(CenterWidgetSize);
			CenterSlot->SetPosition(Center);
			CenterSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			CenterSlot->SetZOrder(10);
		}
	}

	for (int32 Index = 0; Index < Options.Num(); ++Index)
	{
		if (!OptionWidgetClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("[WorldInteraction] OptionWidgetClass is not assigned in %s"), *GetName());
			break;
		}
		UWorldInteractionRadialOptionWidget* OptionWidget = CreateWidget<UWorldInteractionRadialOptionWidget>(this, OptionWidgetClass);
		if (!OptionWidget) continue;

		OptionWidget->SetupOption(InController, Options[Index]);
		UCanvasPanelSlot* OptionCanvasSlot = Canvas_Root->AddChildToCanvas(OptionWidget);
		const float AngleRadians = FMath::DegreesToRadians(-90.f + (360.f * Index / Options.Num()));
		OptionCanvasSlot->SetAnchors(FAnchors(0.f, 0.f));
		OptionCanvasSlot->SetAutoSize(false);
		OptionCanvasSlot->SetSize(OptionWidgetSize);
		OptionCanvasSlot->SetPosition(Center + FVector2D(FMath::Cos(AngleRadians), FMath::Sin(AngleRadians)) * OptionRadius);
		OptionCanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		OptionCanvasSlot->SetZOrder(20);
		SpawnedOptions.Add(OptionWidget);
	}
}

void UWorldInteractionRadialWidget::HandleBackdropClicked()
{
	if (AEidosPlayerController* Controller = OwningController.Get()) Controller->CloseWorldInteractionRadial();
}

void UWorldInteractionRadialWidget::ClearOptionWidgets()
{
	for (UWorldInteractionRadialOptionWidget* OptionWidget : SpawnedOptions)
	{
		if (OptionWidget) OptionWidget->RemoveFromParent();
	}
	SpawnedOptions.Reset();
}

FVector2D UWorldInteractionRadialWidget::ClampMenuCenter(FVector2D DesiredCenter) const
{
	const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(const_cast<UWorldInteractionRadialWidget*>(this));
	if (ViewportSize.IsNearlyZero()) return DesiredCenter;
	const FVector2D RingExtent = FVector2D(
		OptionRadius + (OptionWidgetSize.X * 0.5f),
		OptionRadius + (OptionWidgetSize.Y * 0.5f));
	return FVector2D(
		FMath::Clamp(DesiredCenter.X, EdgePadding + RingExtent.X, ViewportSize.X - EdgePadding - RingExtent.X),
		FMath::Clamp(DesiredCenter.Y, EdgePadding + RingExtent.Y, ViewportSize.Y - EdgePadding - RingExtent.Y));
}
