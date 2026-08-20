// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/HUD/Panels/Panel_Pages.h"
#include "Framework/EidosPlayerController.h"
#include "World/Settlement/WS_Population.h"
#include "Combat/WS_CombatDirector.h"
#include "Entities/Page/Components/StatsComponent.h"
#include "Entities/Page/Components/SkillComponent.h"
#include "Data/GIS_DataRegistry.h"
#include "Data/Definitions/SkillDefinitionRow.h"
#include "UI/HUD/Panels/PageEntry.h"
#include "UI/HUD/Panels/PageQuickbarSlot.h"
#include "UI/HUD/EidosHUD.h"
#include "UI/HUD/HUDRootWidget.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Engine/World.h"
#include "TimerManager.h"

namespace
{
	FText BuildPageStatusText(APageCharacter* Page)
	{
		if (!Page)
		{
			return FText::GetEmpty();
		}

		if (UStatsComponent* Stats = Page->GetStats())
		{
			if (Stats->IsDead())
			{
				return FText::FromString(TEXT("사망"));
			}
			if (Stats->IsDowned())
			{
				return FText::Format(FText::FromString(TEXT("기절 | 구조까지 {0}초")), FMath::CeilToInt(Stats->GetDownedTimeRemaining()));
			}
			if (Stats->IsRecovering())
			{
				return FText::Format(FText::FromString(TEXT("회복 중 | 행동 가능까지 {0}초")), FMath::CeilToInt(Stats->GetRecoveryTimeRemaining()));
			}
		}

		FText BaseStatus;
	if (Page->IsInDungeon())
		{
			BaseStatus = FText::FromString(TEXT("던전 내"));
		}

		if (Page->IsCaptive())
		{
			return FText::FromString(TEXT("수감 중"));
		}
		else if (Page->IsInTurnCombat())
		{
			BaseStatus = Page->HasActiveCombatTurn() ? FText::FromString(TEXT("전투 중 (행동 가능)")) : FText::FromString(TEXT("전투 중"));
		}
		else if (Page->IsManualWorkOverrideActive())
		{
			BaseStatus = FText::FromString(TEXT("수동 조작 중"));
		}
		else if (Page->CurrentJobState.bIsActive)
		{
			BaseStatus = FText::Format(FText::FromString(TEXT("{0} 작업 중")), FText::FromName(Page->CurrentJobState.WorkId));
		}
		else
		{
			BaseStatus = FText::FromString(TEXT("대기 중"));
		}

		FString StatusSuffix;
		if (Page->IsSettlementOverCapacity())
		{
			StatusSuffix += TEXT("수용량 초과");
		}
		if (Page->HasSettlementFoodShortage())
		{
			const FString StarvationText = FString::Printf(TEXT("굶주림 %d%%"), FMath::RoundToInt(Page->GetSettlementStarvationSeverity() * 100.f));
			StatusSuffix += StatusSuffix.IsEmpty() ? StarvationText : TEXT(" | ") + StarvationText;
		}

		return StatusSuffix.IsEmpty()
			? BaseStatus
			: FText::Format(FText::FromString(TEXT("{0} | {1}")), BaseStatus, FText::FromString(StatusSuffix));
	}

	void AutoPopulateQuickBarIfNeeded(APageCharacter* Page, UGIS_DataRegistry* Registry)
	{
		if (!Page || !Registry || !Page->Skills)
		{
			return;
		}

		bool bHasAssignedSlot = false;
		const TArray<FPageCombatActionSlot>& ExistingSlots = Page->GetCombatActionSlots();
		for (const FPageCombatActionSlot& Slot : ExistingSlots)
		{
			if (Slot.ActionType != EPageCombatActionType::None)
			{
				bHasAssignedSlot = true;
				break;
			}
		}

		if (bHasAssignedSlot)
		{
			return;
		}

		struct FSkillCandidate
		{
			FName SkillId;
			FText DisplayName;
		};

		TArray<FSkillCandidate> Candidates;
		for (const TPair<FName, FPageSkillRuntime>& Pair : Page->Skills->GetAllSkillStates())
		{
			const FSkillDefinitionRow* SkillDef = Registry->GetSkillDef(Pair.Key);
			if (!SkillDef || !SkillDef->bIsActiveCombatSkill)
			{
				continue;
			}

			FSkillCandidate Candidate;
			Candidate.SkillId = Pair.Key;
			Candidate.DisplayName = SkillDef->DisplayName;
			Candidates.Add(Candidate);
		}

		Candidates.Sort([](const FSkillCandidate& A, const FSkillCandidate& B)
		{
			return A.DisplayName.ToString() < B.DisplayName.ToString();
		});

		for (int32 SlotIndex = 0; SlotIndex < 10; ++SlotIndex)
		{
			FPageCombatActionSlot SlotData;
			if (Candidates.IsValidIndex(SlotIndex))
			{
				SlotData.ActionType = EPageCombatActionType::ActiveSkill;
				SlotData.ActionId = Candidates[SlotIndex].SkillId;
				SlotData.DisplayName = Candidates[SlotIndex].DisplayName;
			}
			Page->SetCombatActionSlot(SlotIndex, SlotData);
		}
	}

	void BuildAvailableActions(APageCharacter* Page, UGIS_DataRegistry* Registry, const TArray<FPageQuickSlotView>& QuickSlots, TArray<FPageActionCandidateView>& OutActions)
	{
		OutActions.Reset();
		if (!Page || !Registry || !Page->Skills)
		{
			return;
		}

		TSet<FName> AssignedSkillIds;
		for (const FPageQuickSlotView& SlotView : QuickSlots)
		{
			if (SlotView.ActionType == EPageCombatActionType::ActiveSkill && !SlotView.ActionId.IsNone())
			{
				AssignedSkillIds.Add(SlotView.ActionId);
			}
		}

		for (const TPair<FName, FPageSkillRuntime>& Pair : Page->Skills->GetAllSkillStates())
		{
			const FSkillDefinitionRow* SkillDef = Registry->GetSkillDef(Pair.Key);
			if (!SkillDef || !SkillDef->bIsActiveCombatSkill)
			{
				continue;
			}

			FPageActionCandidateView Candidate;
			Candidate.ActionType = EPageCombatActionType::ActiveSkill;
			Candidate.ActionId = Pair.Key;
			Candidate.DisplayName = SkillDef->DisplayName;
			Candidate.ActionPointCost = SkillDef->CombatActionPointCost;
			Candidate.bAssignedToQuickBar = AssignedSkillIds.Contains(Pair.Key);
			OutActions.Add(Candidate);
		}

		OutActions.Sort([](const FPageActionCandidateView& A, const FPageActionCandidateView& B)
		{
			return A.DisplayName.ToString() < B.DisplayName.ToString();
		});
	}
}

void UPanel_Pages::NativeConstruct()
{
	Super::NativeConstruct();

	// Nested UMG instances can retain an older empty class override. Recover the
	// project's default slot presentation so the panel never falls back to raw text.
	if (!QuickbarSlotClass)
	{
		QuickbarSlotClass = LoadClass<UPageQuickbarSlot>(
			nullptr,
			TEXT("/Game/Blueprints/WBP/WBP_PageQuickBarSlot.WBP_PageQuickBarSlot_C"));
	}

	UE_LOG(LogTemp, Log, TEXT("[Pages] NativeConstruct Class=%s QuickbarSlotClass=%s"),
		*GetClass()->GetPathName(),
		*GetNameSafe(QuickbarSlotClass));

	BindWorldActorDestroyed();

	if (Button_Details)
	{
		Button_Details->OnClicked.AddDynamic(this, &UPanel_Pages::HandleDetailsClicked);
	}

	if (Button_EditSkills)
	{
		Button_EditSkills->OnClicked.AddDynamic(this, &UPanel_Pages::HandleEditSkillsClicked);
	}
	if (Button_Equipment)
	{
		Button_Equipment->OnClicked.AddDynamic(this, &UPanel_Pages::HandleEquipmentClicked);
	}
	if (Button_WorkPriorities)
	{
		Button_WorkPriorities->OnClicked.AddDynamic(this, &UPanel_Pages::HandleWorkPrioritiesClicked);
	}
	if (Button_ExpeditionRoster)
	{
		Button_ExpeditionRoster->OnClicked.AddDynamic(this, &UPanel_Pages::HandleExpeditionRosterClicked);
	}
	if (Button_RecruitCaptive)
	{
		Button_RecruitCaptive->OnClicked.AddDynamic(this, &UPanel_Pages::HandleRecruitCaptiveClicked);
		UE_LOG(LogTemp, Log, TEXT("[Pages] Recruit button bound: %s"), *Button_RecruitCaptive->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Pages] Button_RecruitCaptive is not bound. Check its exact name and Is Variable setting in WBP_Panel_Pages."));
	}

	RefreshFromWorld();
}

void UPanel_Pages::NativeDestruct()
{
	UnbindWorldActorDestroyed();
	UnbindObservedStats();
	Super::NativeDestruct();
}

void UPanel_Pages::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!InspectedCaptive.IsValid())
	{
		CaptiveDetailRefreshElapsed = 0.f;
		return;
	}

	CaptiveDetailRefreshElapsed += InDeltaTime;
	if (CaptiveDetailRefreshElapsed >= 0.25f)
	{
		CaptiveDetailRefreshElapsed = 0.f;
		RefreshSelectedPageVitals();
	}
}

void UPanel_Pages::OnPanelShown_Implementation()
{
	RefreshFromWorld();
}

void UPanel_Pages::OnPanelHidden_Implementation()
{
	UnbindObservedStats();
}

void UPanel_Pages::RefreshFromWorld()
{
	CachedPages.Reset();
	SelectedPageQuickSlots.Reset();
	SelectedPageAvailableActions.Reset();
	SelectedPageSummary = FPageSummaryView();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UWS_Population* Population = World->GetSubsystem<UWS_Population>();
	AEidosPlayerController* EidosPC = Cast<AEidosPlayerController>(GetOwningPlayer());
	UWS_CombatDirector* CombatDirector = World->GetSubsystem<UWS_CombatDirector>();
	APageCharacter* SelectedPage = EidosPC ? EidosPC->GetSelectedPage() : nullptr;
	UGIS_DataRegistry* Registry = World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UGIS_DataRegistry>() : nullptr;
	if (!Population)
	{
		return;
	}

	if (SelectedPage)
	{
		AutoPopulateQuickBarIfNeeded(SelectedPage, Registry);
	}

	APageCharacter* DetailPage = InspectedCaptive.Get();
	if (!DetailPage || !DetailPage->IsCaptive())
	{
		InspectedCaptive.Reset();
		DetailPage = SelectedPage;
	}

	RebindSelectedPageStats(DetailPage);

	auto AddPageView = [this, CombatDirector, DetailPage](APageCharacter* Page)
	{
		if (!Page)
		{
			return;
		}

		FPageSummaryView View;
		View.PageId = Page->GetPageEntityId();
		View.DisplayName = FText::FromString(GetNameSafe(Page));
		View.Faction = Page->GetFaction();
		View.bIsSelected = (Page == DetailPage);
		View.bIsCaptive = Page->IsCaptive();
		View.bIsInDungeon = Page->IsInDungeon();
		View.bIsInTurnCombat = Page->IsInTurnCombat();
		View.bHasActiveCombatTurn = Page->HasActiveCombatTurn();
		View.StatusText = BuildPageStatusText(Page);
		View.CurrentInventoryVolume = Page->GetCurrentInventoryVolume();
		View.MaxInventoryVolume = Page->GetMaxInventoryVolume();
		View.CurrentInventoryWeight = Page->GetCurrentInventoryWeight();
		View.MaxInventoryWeight = Page->GetMaxInventoryWeight();

		if (UStatsComponent* Stats = Page->GetStats())
		{
			View.bIsDead = Stats->IsDead();
			View.Health = Stats->GetHealth();
			View.MaxHealth = Stats->GetMaxHealth();
			View.Hunger = Stats->GetHunger();
			View.Fatigue = Stats->GetFatigue();
		}

		if (CombatDirector)
		{
			View.ActionPointsRemaining = CombatDirector->GetActionPointsRemaining(Page);
		}

		CachedPages.Add(View);

		if (Page == DetailPage)
		{
			SelectedPageSummary = View;
		}
	};

	for (const TWeakObjectPtr<APageCharacter>& WeakPage : Population->GetOwnedPages())
	{
		AddPageView(WeakPage.Get());
	}

	TArray<APageCharacter*> Captives;
	Population->GetCaptivePages(Captives);
	for (APageCharacter* Captive : Captives)
	{
		AddPageView(Captive);
	}

	if (DetailPage && !DetailPage->IsCaptive())
	{
		for (int32 SlotIndex = 0; SlotIndex < 10; ++SlotIndex)
		{
			FPageCombatActionSlot ActionSlot;
			FPageQuickSlotView SlotView;
			SlotView.SlotIndex = SlotIndex;
			SlotView.SlotLabel = FText::FromString(SlotIndex == 9 ? TEXT("0") : FString::FromInt(SlotIndex + 1));
			if (DetailPage->GetCombatActionSlot(SlotIndex, ActionSlot))
			{
				SlotView.ActionType = ActionSlot.ActionType;
				SlotView.ActionId = ActionSlot.ActionId;
				SlotView.DisplayName = ActionSlot.DisplayName;
				SlotView.bAssigned = ActionSlot.ActionType != EPageCombatActionType::None;
			}
			SelectedPageQuickSlots.Add(SlotView);
		}

		BuildAvailableActions(DetailPage, Registry, SelectedPageQuickSlots, SelectedPageAvailableActions);
	}

	RebuildPageEntryWidgets();
	RefreshDetailWidgets();
}

bool UPanel_Pages::RequestSelectPage(int32 PageId)
{
	if (UWorld* World = GetWorld())
	{
		if (UWS_Population* Population = World->GetSubsystem<UWS_Population>())
		{
			if (APageCharacter* Captive = Population->FindCaptiveById(PageId))
			{
				InspectedCaptive = Captive;
				RefreshFromWorld();
				return true;
			}
		}
	}

	if (AEidosPlayerController* EidosPC = Cast<AEidosPlayerController>(GetOwningPlayer()))
	{
		InspectedCaptive.Reset();
		const bool bChanged = EidosPC->SelectPageByEntityId(PageId);
		if (bChanged)
		{
			RefreshFromWorld();
		}
		return bChanged;
	}

	return false;
}

bool UPanel_Pages::AssignSelectedPageQuickSlot(int32 SlotIndex, const FPageCombatActionSlot& SlotData)
{
	AEidosPlayerController* EidosPC = Cast<AEidosPlayerController>(GetOwningPlayer());
	APageCharacter* SelectedPage = EidosPC ? EidosPC->GetSelectedPage() : nullptr;
	if (!SelectedPage)
	{
		return false;
	}

	if (!SlotData.ActionId.IsNone())
	{
		const TArray<FPageCombatActionSlot>& ExistingSlots = SelectedPage->GetCombatActionSlots();
		for (int32 ExistingIndex = 0; ExistingIndex < ExistingSlots.Num(); ++ExistingIndex)
		{
			const FPageCombatActionSlot& Existing = ExistingSlots[ExistingIndex];
			if (ExistingIndex != SlotIndex && Existing.ActionType == SlotData.ActionType && Existing.ActionId == SlotData.ActionId)
			{
				SelectedPage->SetCombatActionSlot(ExistingIndex, FPageCombatActionSlot{});
			}
		}
	}

	SelectedPage->SetCombatActionSlot(SlotIndex, SlotData);
	RefreshFromWorld();
	return true;
}

bool UPanel_Pages::ClearSelectedPageQuickSlot(int32 SlotIndex)
{
	AEidosPlayerController* EidosPC = Cast<AEidosPlayerController>(GetOwningPlayer());
	APageCharacter* SelectedPage = EidosPC ? EidosPC->GetSelectedPage() : nullptr;
	if (!SelectedPage)
	{
		return false;
	}

	SelectedPage->SetCombatActionSlot(SlotIndex, FPageCombatActionSlot{});
	RefreshFromWorld();
	return true;
}

void UPanel_Pages::RebuildPageEntryWidgets()
{
	if (!VerticalBox_PageEntries)
	{
		return;
	}

	VerticalBox_PageEntries->ClearChildren();
	if (!PageEntryClass)
	{
		return;
	}

	for (const FPageSummaryView& PageView : CachedPages)
	{
		UPageEntry* Entry = CreateWidget<UPageEntry>(this, PageEntryClass);
		if (!Entry)
		{
			continue;
		}

		Entry->Setup(PageView);
		Entry->OnEntryClicked.AddDynamic(this, &UPanel_Pages::HandlePageEntryClicked);
		VerticalBox_PageEntries->AddChild(Entry);
	}
}

void UPanel_Pages::RefreshDetailWidgets()
{
	const bool bHasSelectedPage = SelectedPageSummary.PageId != INDEX_NONE;

	if (Text_SelectedPageName)
	{
		Text_SelectedPageName->SetText(bHasSelectedPage ? SelectedPageSummary.DisplayName : FText::FromString(TEXT("선택된 Page 없음")));
	}

	if (ProgressBar_HP)
	{
		const float Percent = bHasSelectedPage && SelectedPageSummary.MaxHealth > 0.f
			? FMath::Clamp(SelectedPageSummary.Health / SelectedPageSummary.MaxHealth, 0.f, 1.f)
			: 0.f;
		ProgressBar_HP->SetPercent(Percent);
	}

	if (Text_HPValue)
	{
		Text_HPValue->SetText(bHasSelectedPage
			? FText::Format(FText::FromString(TEXT("HP {0} / {1}")), FMath::RoundToInt(SelectedPageSummary.Health), FMath::RoundToInt(SelectedPageSummary.MaxHealth))
			: FText::GetEmpty());
	}

	if (Text_Status)
	{
		FText StatusText = bHasSelectedPage ? SelectedPageSummary.StatusText : FText::GetEmpty();
		if (bHasSelectedPage && SelectedPageSummary.bIsCaptive)
		{
			if (UWorld* World = GetWorld())
			{
				if (UWS_Population* Population = World->GetSubsystem<UWS_Population>())
				{
					if (APageCharacter* Captive = Population->FindCaptiveById(SelectedPageSummary.PageId))
					{
						const int32 Resistance = FMath::CeilToInt(Captive->GetCaptiveResistance());
						if (Population->IsCaptiveRecruitmentActive(SelectedPageSummary.PageId))
						{
							StatusText = FText::Format(FText::FromString(TEXT("수감 중 | 저항 {0} | 포섭 진행 중")), Resistance);
						}
						else if (Population->IsCaptiveRecruitmentRequested(SelectedPageSummary.PageId))
						{
							StatusText = FText::Format(FText::FromString(TEXT("수감 중 | 저항 {0} | 포섭 대기")), Resistance);
						}
						else
						{
							StatusText = FText::Format(FText::FromString(TEXT("수감 중 | 저항 {0}")), Resistance);
						}
					}
				}
			}
		}
		Text_Status->SetText(StatusText);
	}

	if (Button_RecruitCaptive)
	{
		Button_RecruitCaptive->SetVisibility(SelectedPageSummary.bIsCaptive ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (Text_RecruitCaptive)
	{
		FText ButtonLabel = FText::FromString(TEXT("포섭 시작"));
		if (SelectedPageSummary.bIsCaptive)
		{
			if (UWorld* World = GetWorld())
			{
				if (UWS_Population* Population = World->GetSubsystem<UWS_Population>())
				{
					ButtonLabel = Population->IsCaptiveRecruitmentRequested(SelectedPageSummary.PageId)
						? FText::FromString(TEXT("포섭 중단"))
						: FText::FromString(TEXT("포섭 시작"));
				}
			}
		}
		Text_RecruitCaptive->SetText(ButtonLabel);
	}

	const ESlateVisibility ManagementVisibility = bHasSelectedPage && !SelectedPageSummary.bIsCaptive
		? ESlateVisibility::Visible
		: ESlateVisibility::Collapsed;
	if (Button_Details)
	{
		Button_Details->SetVisibility(ManagementVisibility);
	}
	if (Button_EditSkills)
	{
		Button_EditSkills->SetVisibility(ManagementVisibility);
	}
	if (Button_Equipment)
	{
		Button_Equipment->SetVisibility(ManagementVisibility);
	}
	if (Button_WorkPriorities)
	{
		Button_WorkPriorities->SetVisibility(ManagementVisibility);
	}
	if (Button_ExpeditionRoster)
	{
		Button_ExpeditionRoster->SetVisibility(ManagementVisibility);
		Button_ExpeditionRoster->SetIsEnabled(bHasSelectedPage && !SelectedPageSummary.bIsDead && !SelectedPageSummary.bIsInDungeon);
	}
	if (Text_ExpeditionRoster)
	{
		bool bRostered = false;
		if (bHasSelectedPage)
		{
			if (UWorld* World = GetWorld())
			{
				if (UWS_Population* Population = World->GetSubsystem<UWS_Population>())
				{
					bRostered = Population->IsPageInExpeditionRoster(SelectedPageSummary.PageId);
				}
			}
		}
		Text_ExpeditionRoster->SetText(bRostered ? FText::FromString(TEXT("원정: 편성됨")) : FText::FromString(TEXT("원정: 미편성")));
	}

	if (Text_Volume)
	{
		Text_Volume->SetText(bHasSelectedPage
			? FText::Format(FText::FromString(TEXT("부피 {0} / {1}")), FMath::RoundToInt(SelectedPageSummary.CurrentInventoryVolume), FMath::RoundToInt(SelectedPageSummary.MaxInventoryVolume))
			: FText::GetEmpty());
	}

	if (Text_Weight)
	{
		Text_Weight->SetText(bHasSelectedPage
			? FText::Format(FText::FromString(TEXT("무게 {0} / {1}")), FMath::RoundToInt(SelectedPageSummary.CurrentInventoryWeight), FMath::RoundToInt(SelectedPageSummary.MaxInventoryWeight))
			: FText::GetEmpty());
	}

	RebuildQuickbarWidgets();
}

void UPanel_Pages::RebuildQuickbarWidgets()
{
	if (!UniformGrid_Quickbar && !WrapBox_Quickbar)
	{
		return;
	}

	if (UniformGrid_Quickbar)
	{
		UniformGrid_Quickbar->ClearChildren();
	}
	else
	{
		WrapBox_Quickbar->ClearChildren();
	}

	const int32 Columns = FMath::Max(1, QuickbarColumns);
	UE_LOG(LogTemp, Log, TEXT("[Pages] RebuildQuickbar Grid=%d SlotClass=%s"),
		UniformGrid_Quickbar ? 1 : 0,
		*GetNameSafe(QuickbarSlotClass));

	for (const FPageQuickSlotView& SlotView : SelectedPageQuickSlots)
	{
		if (QuickbarSlotClass)
		{
			if (UPageQuickbarSlot* SlotWidget = CreateWidget<UPageQuickbarSlot>(this, QuickbarSlotClass))
			{
				SlotWidget->Setup(SlotView);
				if (UniformGrid_Quickbar)
				{
					if (UUniformGridSlot* GridSlot = UniformGrid_Quickbar->AddChildToUniformGrid(
						SlotWidget,
						SlotView.SlotIndex / Columns,
						SlotView.SlotIndex % Columns))
					{
						GridSlot->SetHorizontalAlignment(HAlign_Fill);
						GridSlot->SetVerticalAlignment(VAlign_Fill);
					}
				}
				else if (UWrapBoxSlot* WrapSlot = WrapBox_Quickbar->AddChildToWrapBox(SlotWidget))
				{
					WrapSlot->SetPadding(FMargin(0.f, 0.f, 6.f, 6.f));
				}
			}
			continue;
		}

		// Keep the quickbar inspectable before its dedicated WBP presentation is created.
		UE_LOG(LogTemp, Warning, TEXT("[Pages] Quickbar slot fallback used for slot %d. SlotClass=%s"),
			SlotView.SlotIndex + 1,
			*GetNameSafe(QuickbarSlotClass));

		UTextBlock* FallbackSlot = NewObject<UTextBlock>(this);
		FallbackSlot->SetJustification(ETextJustify::Center);
		FallbackSlot->SetText(FText::Format(
			FText::FromString(TEXT("[{0}]\n{1}")),
			SlotView.SlotLabel,
			SlotView.bAssigned ? SlotView.DisplayName : FText::GetEmpty()));
		if (UniformGrid_Quickbar)
		{
			if (UUniformGridSlot* GridSlot = UniformGrid_Quickbar->AddChildToUniformGrid(
				FallbackSlot,
				SlotView.SlotIndex / Columns,
				SlotView.SlotIndex % Columns))
			{
				GridSlot->SetHorizontalAlignment(HAlign_Fill);
				GridSlot->SetVerticalAlignment(VAlign_Fill);
			}
		}
		else if (UWrapBoxSlot* WrapSlot = WrapBox_Quickbar->AddChildToWrapBox(FallbackSlot))
		{
			WrapSlot->SetPadding(FMargin(0.f, 0.f, 6.f, 6.f));
		}
	}
}

void UPanel_Pages::RebindSelectedPageStats(APageCharacter* SelectedPage)
{
	UStatsComponent* NewStats = SelectedPage ? SelectedPage->GetStats() : nullptr;
	if (ObservedStats.Get() == NewStats)
	{
		return;
	}

	UnbindObservedStats();
	if (NewStats)
	{
		ObservedStats = NewStats;
		NewStats->OnStatsChanged.AddDynamic(this, &UPanel_Pages::HandleSelectedPageStatsChanged);
	}
}

void UPanel_Pages::UnbindObservedStats()
{
	if (UStatsComponent* Stats = ObservedStats.Get())
	{
		Stats->OnStatsChanged.RemoveDynamic(this, &UPanel_Pages::HandleSelectedPageStatsChanged);
	}
	ObservedStats.Reset();
}

void UPanel_Pages::BindWorldActorDestroyed()
{
	if (!ActorDestroyedHandle.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			ActorDestroyedHandle = World->AddOnActorDestroyedHandler(
				FOnActorDestroyed::FDelegate::CreateUObject(this, &UPanel_Pages::HandleWorldActorDestroyed));
		}
	}
}

void UPanel_Pages::UnbindWorldActorDestroyed()
{
	if (ActorDestroyedHandle.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			World->RemoveOnActorDestroyedHandler(ActorDestroyedHandle);
		}
		ActorDestroyedHandle.Reset();
	}
}

void UPanel_Pages::HandlePageEntryClicked(int32 PageId)
{
	RequestSelectPage(PageId);
}

void UPanel_Pages::HandleDetailsClicked()
{
	if (SelectedPageSummary.PageId != INDEX_NONE)
	{
		OnDetailsRequested(SelectedPageSummary);
	}
}

void UPanel_Pages::HandleEditSkillsClicked()
{
	if (SelectedPageSummary.PageId == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Pages] Edit skills ignored because no Page is selected"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[Pages] Edit skills clicked for PageId=%d"), SelectedPageSummary.PageId);

	if (AEidosPlayerController* EidosPC = Cast<AEidosPlayerController>(GetOwningPlayer()))
	{
		if (AEidosHUD* EidosHUD = Cast<AEidosHUD>(EidosPC->GetHUD()))
		{
			if (UHUDRootWidget* HUDRoot = EidosHUD->GetHUDRootWidget())
			{
				HUDRoot->ShowPageSkillEditor(this);
				return;
			}
		}
	}

	OnEditSkillsRequested(SelectedPageSummary);
}

void UPanel_Pages::HandleEquipmentClicked()
{
	if (SelectedPageSummary.PageId == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Pages] Equipment editor ignored because no Page is selected"));
		return;
	}

	if (AEidosPlayerController* EidosPC = Cast<AEidosPlayerController>(GetOwningPlayer()))
	{
		if (AEidosHUD* EidosHUD = Cast<AEidosHUD>(EidosPC->GetHUD()))
		{
			if (UHUDRootWidget* HUDRoot = EidosHUD->GetHUDRootWidget())
			{
				HUDRoot->ShowPageEquipmentEditor(EidosPC->GetSelectedPage());
			}
		}
	}
}

void UPanel_Pages::HandleWorkPrioritiesClicked()
{
	if (SelectedPageSummary.PageId == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Pages] Work priority editor ignored because no Page is selected"));
		return;
	}

	if (AEidosPlayerController* EidosPC = Cast<AEidosPlayerController>(GetOwningPlayer()))
	{
		if (AEidosHUD* EidosHUD = Cast<AEidosHUD>(EidosPC->GetHUD()))
		{
			if (UHUDRootWidget* HUDRoot = EidosHUD->GetHUDRootWidget())
			{
				HUDRoot->ShowPageWorkPriorityEditor(EidosPC->GetSelectedPage());
			}
		}
	}
}

void UPanel_Pages::HandleExpeditionRosterClicked()
{
	if (SelectedPageSummary.PageId == INDEX_NONE)
	{
		return;
	}

	UWS_Population* Population = GetWorld() ? GetWorld()->GetSubsystem<UWS_Population>() : nullptr;
	FString Reason;
	if (!Population || !Population->TogglePageExpeditionRoster(SelectedPageSummary.PageId, Reason))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Pages] Expedition roster update failed PageId=%d Reason=%s"), SelectedPageSummary.PageId, *Reason);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[Pages] Expedition roster updated PageId=%d: %s"), SelectedPageSummary.PageId, *Reason);
	RefreshDetailWidgets();
}

void UPanel_Pages::HandleRecruitCaptiveClicked()
{
	UE_LOG(LogTemp, Log, TEXT("[Pages] Recruit clicked PageId=%d IsCaptive=%d"), SelectedPageSummary.PageId, SelectedPageSummary.bIsCaptive);
	if (!SelectedPageSummary.bIsCaptive || SelectedPageSummary.PageId == INDEX_NONE)
	{
		return;
	}

	UWorld* World = GetWorld();
	UWS_Population* Population = World ? World->GetSubsystem<UWS_Population>() : nullptr;
	FString Reason;
	if (!Population || !Population->ToggleCaptiveRecruitment(SelectedPageSummary.PageId, Reason))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Pages] Recruit captive toggle failed PageId=%d Reason=%s"), SelectedPageSummary.PageId, *Reason);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[Pages] Recruitment state changed PageId=%d: %s"), SelectedPageSummary.PageId, *Reason);
	RefreshSelectedPageVitals();
}

void UPanel_Pages::HandleSelectedPageStatsChanged()
{
	// Hunger and fatigue update continuously in the simulation. Rebuilding the
	// entry list here destroys the widget under the cursor every tick, which
	// makes its hover state flicker and can swallow its click.
	RefreshSelectedPageVitals();
}

void UPanel_Pages::RefreshSelectedPageVitals()
{
	AEidosPlayerController* EidosPC = Cast<AEidosPlayerController>(GetOwningPlayer());
	APageCharacter* SelectedPage = InspectedCaptive.IsValid() ? InspectedCaptive.Get() : (EidosPC ? EidosPC->GetSelectedPage() : nullptr);
	if (!SelectedPage || SelectedPage->GetPageEntityId() != SelectedPageSummary.PageId)
	{
		// A different Page was selected outside this panel; rebuild once to make
		// the list selection and the detail view agree again.
		RefreshFromWorld();
		return;
	}
	if (InspectedCaptive.IsValid() && !SelectedPage->IsCaptive())
	{
		// Recruitment completed while this entry was open. Rebuild once so the
		// former captive becomes a normal controllable Page entry.
		InspectedCaptive.Reset();
		RefreshFromWorld();
		return;
	}

	SelectedPageSummary.StatusText = BuildPageStatusText(SelectedPage);
	SelectedPageSummary.bIsCaptive = SelectedPage->IsCaptive();
	SelectedPageSummary.bIsInDungeon = SelectedPage->IsInDungeon();
	SelectedPageSummary.bIsInTurnCombat = SelectedPage->IsInTurnCombat();
	SelectedPageSummary.bHasActiveCombatTurn = SelectedPage->HasActiveCombatTurn();
	SelectedPageSummary.CurrentInventoryVolume = SelectedPage->GetCurrentInventoryVolume();
	SelectedPageSummary.MaxInventoryVolume = SelectedPage->GetMaxInventoryVolume();
	SelectedPageSummary.CurrentInventoryWeight = SelectedPage->GetCurrentInventoryWeight();
	SelectedPageSummary.MaxInventoryWeight = SelectedPage->GetMaxInventoryWeight();

	if (UStatsComponent* Stats = SelectedPage->GetStats())
	{
		SelectedPageSummary.bIsDead = Stats->IsDead();
		SelectedPageSummary.Health = Stats->GetHealth();
		SelectedPageSummary.MaxHealth = Stats->GetMaxHealth();
		SelectedPageSummary.Hunger = Stats->GetHunger();
		SelectedPageSummary.Fatigue = Stats->GetFatigue();
	}

	if (Text_HPValue)
	{
		Text_HPValue->SetText(FText::Format(
			FText::FromString(TEXT("HP {0} / {1}")),
			FMath::RoundToInt(SelectedPageSummary.Health),
			FMath::RoundToInt(SelectedPageSummary.MaxHealth)));
	}

	if (ProgressBar_HP)
	{
		const float Percent = SelectedPageSummary.MaxHealth > 0.f
			? FMath::Clamp(SelectedPageSummary.Health / SelectedPageSummary.MaxHealth, 0.f, 1.f)
			: 0.f;
		ProgressBar_HP->SetPercent(Percent);
	}

	if (Text_Status)
	{
		if (SelectedPageSummary.bIsCaptive)
		{
			RefreshDetailWidgets();
			return;
		}
		Text_Status->SetText(SelectedPageSummary.StatusText);
	}

	if (Text_Volume)
	{
		Text_Volume->SetText(FText::Format(FText::FromString(TEXT("부피 {0} / {1}")),
			FMath::RoundToInt(SelectedPageSummary.CurrentInventoryVolume),
			FMath::RoundToInt(SelectedPageSummary.MaxInventoryVolume)));
	}

	if (Text_Weight)
	{
		Text_Weight->SetText(FText::Format(FText::FromString(TEXT("무게 {0} / {1}")),
			FMath::RoundToInt(SelectedPageSummary.CurrentInventoryWeight),
			FMath::RoundToInt(SelectedPageSummary.MaxInventoryWeight)));
	}
}

void UPanel_Pages::HandleWorldActorDestroyed(AActor* DestroyedActor)
{
	APageCharacter* DestroyedPage = Cast<APageCharacter>(DestroyedActor);
	if (!DestroyedPage || !DestroyedPage->IsFriendly())
	{
		return;
	}

	RefreshFromWorld();

	// The player controller may select a replacement Page during destruction.
	// Refresh once more after that selection and the actor removal are complete.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				RefreshFromWorld();
			}));
	}
}
