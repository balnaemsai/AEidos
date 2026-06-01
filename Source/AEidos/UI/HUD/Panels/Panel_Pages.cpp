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
#include "Components/WrapBox.h"

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
		}

		if (Page->IsInDungeon())
		{
			return FText::FromString(TEXT("던전 내"));
		}

		if (Page->IsInTurnCombat())
		{
			return Page->HasActiveCombatTurn() ? FText::FromString(TEXT("전투 중 (행동 가능)")) : FText::FromString(TEXT("전투 중"));
		}

		if (Page->CurrentJobState.bIsActive)
		{
			return FText::Format(FText::FromString(TEXT("{0} 작업 중")), FText::FromName(Page->CurrentJobState.WorkId));
		}

		return FText::FromString(TEXT("대기 중"));
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
	RefreshFromWorld();
}

void UPanel_Pages::OnPanelShown_Implementation()
{
	RefreshFromWorld();
}

void UPanel_Pages::OnPanelHidden_Implementation()
{
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

	for (const TWeakObjectPtr<APageCharacter>& WeakPage : Population->GetOwnedPages())
	{
		APageCharacter* Page = WeakPage.Get();
		if (!Page)
		{
			continue;
		}

		FPageSummaryView View;
		View.PageId = Page->GetPageEntityId();
		View.DisplayName = FText::FromString(GetNameSafe(Page));
		View.Faction = Page->GetFaction();
		View.bIsSelected = (Page == SelectedPage);
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

		if (Page == SelectedPage)
		{
			SelectedPageSummary = View;
		}
	}

	if (SelectedPage)
	{
		for (int32 SlotIndex = 0; SlotIndex < 10; ++SlotIndex)
		{
			FPageCombatActionSlot ActionSlot;
			FPageQuickSlotView SlotView;
			SlotView.SlotIndex = SlotIndex;
			SlotView.SlotLabel = FText::FromString(SlotIndex == 9 ? TEXT("0") : FString::FromInt(SlotIndex + 1));
			if (SelectedPage->GetCombatActionSlot(SlotIndex, ActionSlot))
			{
				SlotView.ActionType = ActionSlot.ActionType;
				SlotView.ActionId = ActionSlot.ActionId;
				SlotView.DisplayName = ActionSlot.DisplayName;
				SlotView.bAssigned = ActionSlot.ActionType != EPageCombatActionType::None;
			}
			SelectedPageQuickSlots.Add(SlotView);
		}

		BuildAvailableActions(SelectedPage, Registry, SelectedPageQuickSlots, SelectedPageAvailableActions);
	}

	RebuildPageEntryWidgets();
}

bool UPanel_Pages::RequestSelectPage(int32 PageId)
{
	if (AEidosPlayerController* EidosPC = Cast<AEidosPlayerController>(GetOwningPlayer()))
	{
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
	if (!WrapBox_PageEntries)
	{
		return;
	}

	WrapBox_PageEntries->ClearChildren();
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
		WrapBox_PageEntries->AddChild(Entry);
	}
}

void UPanel_Pages::HandlePageEntryClicked(int32 PageId)
{
	RequestSelectPage(PageId);
}
