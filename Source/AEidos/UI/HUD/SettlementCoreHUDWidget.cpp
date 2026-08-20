#include "UI/HUD/SettlementCoreHUDWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "World/Settlement/SettlementCoreActor.h"
#include "World/Settlement/WS_SettlementCore.h"

void USettlementCoreHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshCoreReadout();
}

void USettlementCoreHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshAccumulator += InDeltaTime;
	if (RefreshAccumulator >= RefreshIntervalSeconds)
	{
		RefreshAccumulator = 0.f;
		RefreshCoreReadout();
	}
}

void USettlementCoreHUDWidget::RefreshCoreReadout()
{
	UWS_SettlementCore* CoreSystem = GetWorld() ? GetWorld()->GetSubsystem<UWS_SettlementCore>() : nullptr;
	ASettlementCoreActor* Core = CoreSystem ? CoreSystem->GetSettlementCore() : nullptr;
	if (!Core)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	const float Health = Core->GetHealth();
	const float MaxHealth = FMath::Max(1.f, Core->GetMaxHealth());
	if (Text_CoreLabel)
	{
		Text_CoreLabel->SetText(Core->IsDestroyed() ? FText::FromString(TEXT("SETTLEMENT CORE DESTROYED")) : FText::FromString(TEXT("SETTLEMENT CORE")));
	}
	if (ProgressBar_CoreHealth)
	{
		ProgressBar_CoreHealth->SetPercent(Health / MaxHealth);
	}
	if (Text_CoreHealth)
	{
		Text_CoreHealth->SetText(FText::Format(FText::FromString(TEXT("{0} / {1}")), FMath::RoundToInt(Health), FMath::RoundToInt(MaxHealth)));
	}
}
