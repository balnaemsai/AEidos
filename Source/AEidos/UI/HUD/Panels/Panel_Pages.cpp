// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/Panels/Panel_Pages.h"
#include "Framework/EidosPlayerController.h"
#include "World/Settlement/WS_Population.h"
#include "Combat/WS_CombatDirector.h"
#include "Entities/Page/Components/StatsComponent.h"

void UPanel_Pages::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshFromWorld();
}

void UPanel_Pages::RefreshFromWorld()
{
	CachedPages.Reset();
	SelectedPageQuickSlots.Reset();
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
	if (!Population)
	{
		return;
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
	}
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

