#include "UI/HUD/CombatHUDWidget.h"

#include "Combat/WS_CombatDirector.h"
#include "Data/Definitions/SkillDefinitionRow.h"
#include "Data/GIS_DataRegistry.h"
#include "Entities/Page/Components/SkillComponent.h"
#include "Entities/Page/Components/StatsComponent.h"
#include "Entities/Page/PageCharacter.h"
#include "Framework/EidosPlayerController.h"
#include "UI/HUD/CombatQuickbarSlot.h"
#include "World/Dungeon/DungeonCoreActor.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Engine/GameInstance.h"

void UCombatHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (Button_EndTurn)
	{
		Button_EndTurn->OnClicked.AddDynamic(this, &UCombatHUDWidget::HandleEndTurnClicked);
	}
	RefreshFromCombat();
}

void UCombatHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshAccumulator += InDeltaTime;
	if (RefreshAccumulator >= RefreshIntervalSeconds)
	{
		RefreshAccumulator = 0.f;
		RefreshFromCombat();
	}
}

void UCombatHUDWidget::RefreshFromCombat()
{
	UWorld* World = GetWorld();
	AEidosPlayerController* PC = Cast<AEidosPlayerController>(GetOwningPlayer());
	UWS_CombatDirector* Combat = World ? World->GetSubsystem<UWS_CombatDirector>() : nullptr;
	if (!PC || !Combat || !Combat->IsCombatActive())
	{
		CachedActivePage.Reset();
		// Keep the root ticking so it can detect the next encounter.
		SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		if (SizeBox_TargetReadout) SizeBox_TargetReadout->SetVisibility(ESlateVisibility::Collapsed);
		if (SizeBox_TurnReadout) SizeBox_TurnReadout->SetVisibility(ESlateVisibility::Collapsed);
		if (SizeBox_CombatCommandBar) SizeBox_CombatCommandBar->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (SizeBox_TargetReadout) SizeBox_TargetReadout->SetVisibility(ESlateVisibility::Visible);
	if (SizeBox_TurnReadout) SizeBox_TurnReadout->SetVisibility(ESlateVisibility::Visible);
	APageCharacter* ActivePage = Combat->GetActiveCombatantForUI();
	const bool bPlayerTurn = ActivePage && ActivePage->IsFriendly();
	const int32 CurrentAP = Combat->GetActionPointsRemaining(ActivePage);
	const int32 MaxAP = Combat->GetMaxActionPoints(ActivePage);

	if (Text_TurnState)
	{
		Text_TurnState->SetText(bPlayerTurn ? FText::FromString(TEXT("PLAYER TURN")) : FText::FromString(TEXT("HOSTILE TURN")));
	}
	if (Text_ActivePageName) Text_ActivePageName->SetText(FText::FromString(GetNameSafe(ActivePage)));
	const FText APText = FText::Format(FText::FromString(TEXT("AP {0}/{1}")), CurrentAP, MaxAP);
	if (Text_AP) Text_AP->SetText(APText);
	if (Text_APLarge) Text_APLarge->SetText(APText);

	if (SizeBox_CombatCommandBar)
	{
		SizeBox_CombatCommandBar->SetVisibility(bPlayerTurn ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (Button_EndTurn) Button_EndTurn->SetIsEnabled(bPlayerTurn);
	if (Text_TargetHint)
	{
		Text_TargetHint->SetText(bPlayerTurn
			? PC->GetCombatTargetingHint()
			: FText::FromString(TEXT("WAITING FOR HOSTILE ACTION")));
		if (bPlayerTurn && Text_TargetHint->GetText().IsEmpty())
		{
			Text_TargetHint->SetText(FText::FromString(TEXT("SELECT AN ACTION")));
		}
	}

	RefreshTargetReadout(PC->GetSelectedCombatTarget());
	if (CachedActivePage.Get() != ActivePage)
	{
		CachedActivePage = ActivePage;
		RebuildQuickbar(ActivePage);
	}

	for (int32 SlotIndex = 0; SlotIndex < QuickbarSlotWidgets.Num(); ++SlotIndex)
	{
		if (UCombatQuickbarSlot* SlotWidget = QuickbarSlotWidgets[SlotIndex])
		{
			SlotWidget->Setup(BuildSlotView(ActivePage, SlotIndex));
		}
	}
}

void UCombatHUDWidget::RebuildQuickbar(APageCharacter* ActivePage)
{
	QuickbarSlotWidgets.Reset();
	if (!WrapBox_QuickbarSlots)
	{
		return;
	}

	WrapBox_QuickbarSlots->ClearChildren();
	if (!CombatQuickbarSlotClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CombatHUD] CombatQuickbarSlotClass is not assigned"));
		return;
	}

	for (int32 SlotIndex = 0; SlotIndex < 10; ++SlotIndex)
	{
		UCombatQuickbarSlot* SlotWidget = CreateWidget<UCombatQuickbarSlot>(this, CombatQuickbarSlotClass);
		if (!SlotWidget)
		{
			continue;
		}

		SlotWidget->Setup(BuildSlotView(ActivePage, SlotIndex));
		SlotWidget->OnSlotClicked.AddDynamic(this, &UCombatHUDWidget::HandleSlotClicked);
		if (UWrapBoxSlot* WrapSlot = WrapBox_QuickbarSlots->AddChildToWrapBox(SlotWidget))
		{
			WrapSlot->SetPadding(FMargin(3.f));
		}
		QuickbarSlotWidgets.Add(SlotWidget);
	}
}

FPageQuickSlotView UCombatHUDWidget::BuildSlotView(APageCharacter* ActivePage, int32 SlotIndex) const
{
	FPageQuickSlotView View;
	View.SlotIndex = SlotIndex;
	View.SlotLabel = FText::FromString(SlotIndex == 9 ? TEXT("0") : FString::FromInt(SlotIndex + 1));
	AEidosPlayerController* PC = Cast<AEidosPlayerController>(GetOwningPlayer());
	View.bIsSelected = PC && PC->GetPendingCombatActionSlot() == SlotIndex;
	if (!ActivePage)
	{
		View.DisabledReason = FText::FromString(TEXT("No active Page"));
		return View;
	}

	FPageCombatActionSlot Action;
	if (!ActivePage->GetCombatActionSlot(SlotIndex, Action) || Action.ActionType == EPageCombatActionType::None)
	{
		View.DisabledReason = FText::FromString(TEXT("Empty slot"));
		return View;
	}

	View.ActionType = Action.ActionType;
	View.ActionId = Action.ActionId;
	View.DisplayName = Action.DisplayName.IsEmpty() ? FText::FromName(Action.ActionId) : Action.DisplayName;
	View.bAssigned = true;

	UWorld* World = GetWorld();
	UWS_CombatDirector* Combat = World ? World->GetSubsystem<UWS_CombatDirector>() : nullptr;
	const bool bActivePlayerTurn = Combat && Combat->IsPageTurnActive(ActivePage) && ActivePage->IsFriendly();
	if (!bActivePlayerTurn)
	{
		View.DisabledReason = FText::FromString(TEXT("Not this Page's turn"));
		return View;
	}

	if (Action.ActionType == EPageCombatActionType::EndTurn)
	{
		View.bCanUse = true;
		return View;
	}

	if (Action.ActionType != EPageCombatActionType::ActiveSkill)
	{
		View.DisabledReason = FText::FromString(TEXT("Action is not implemented"));
		return View;
	}

	UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	UGIS_DataRegistry* Registry = GI ? GI->GetSubsystem<UGIS_DataRegistry>() : nullptr;
	const FSkillDefinitionRow* SkillDef = Registry && Registry->EnsureReadySync() ? Registry->GetSkillDef(Action.ActionId) : nullptr;
	if (!SkillDef || !SkillDef->bIsActiveCombatSkill || !ActivePage->Skills || !ActivePage->Skills->HasSkill(Action.ActionId))
	{
		View.DisabledReason = FText::FromString(TEXT("Skill unavailable"));
		return View;
	}

	View.ActionPointCost = SkillDef->CombatActionPointCost;
	View.bRequiresTarget = SkillDef->bRequiresTarget;
	if (Combat->GetActionPointsRemaining(ActivePage) < View.ActionPointCost)
	{
		View.DisabledReason = FText::FromString(TEXT("Not enough AP"));
		return View;
	}
	// Targeted skills stay selectable without a target; selection enters target-pick mode.
	View.bCanUse = true;
	return View;
}

void UCombatHUDWidget::RefreshTargetReadout(AActor* TargetActor)
{
	const bool bHasTarget = IsValid(TargetActor);
	if (SizeBox_TargetReadout) SizeBox_TargetReadout->SetVisibility(ESlateVisibility::Visible);
	if (!bHasTarget)
	{
		if (Text_TargetType) Text_TargetType->SetText(FText::FromString(TEXT("TARGET")));
		if (Text_TargetName) Text_TargetName->SetText(FText::FromString(TEXT("NO TARGET SELECTED")));
		if (ProgressBar_TargetHP) ProgressBar_TargetHP->SetPercent(0.f);
		if (Text_TargetHP) Text_TargetHP->SetText(FText::GetEmpty());
		return;
	}

	float Health = 0.f;
	float MaxHealth = 0.f;
	FText TypeText = FText::FromString(TEXT("TARGET"));
	if (APageCharacter* TargetPage = Cast<APageCharacter>(TargetActor))
	{
		TypeText = FText::FromString(TEXT("HOSTILE PAGE"));
		if (UStatsComponent* Stats = TargetPage->GetStats())
		{
			Health = Stats->GetHealth();
			MaxHealth = Stats->GetMaxHealth();
		}
	}
	else if (ADungeonCoreActor* Core = Cast<ADungeonCoreActor>(TargetActor))
	{
		TypeText = FText::FromString(TEXT("DUNGEON CORE"));
		Health = Core->GetHealth();
		MaxHealth = Core->GetMaxHealth();
	}

	if (Text_TargetType) Text_TargetType->SetText(TypeText);
	if (Text_TargetName) Text_TargetName->SetText(FText::FromString(GetNameSafe(TargetActor)));
	if (ProgressBar_TargetHP) ProgressBar_TargetHP->SetPercent(MaxHealth > 0.f ? Health / MaxHealth : 0.f);
	if (Text_TargetHP) Text_TargetHP->SetText(FText::Format(FText::FromString(TEXT("{0}/{1}")), FMath::RoundToInt(Health), FMath::RoundToInt(MaxHealth)));
}

void UCombatHUDWidget::HandleEndTurnClicked()
{
	if (AEidosPlayerController* PC = Cast<AEidosPlayerController>(GetOwningPlayer()))
	{
		PC->RequestCombatEndTurn();
	}
}

void UCombatHUDWidget::HandleSlotClicked(int32 SlotIndex)
{
	if (AEidosPlayerController* PC = Cast<AEidosPlayerController>(GetOwningPlayer()))
	{
		PC->UseCombatActionSlot(SlotIndex);
	}
}
