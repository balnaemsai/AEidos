// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/BaseHUDWidget.h"
#include "UI/HUD/BaseHUDWidget.h"
#include "Engine/World.h"
#include "UI/HUD/ResourcePanelWidget.h"
#include "UI/HUD/PageRosterWidget.h"
#include "UI/HUD/DungeonStatusWidget.h"
#include "UI/HUD/NotificationFeedWidget.h"
#include "UI/HUD/QuickBarWidget.h"
#include "UI/HUD/HUDViewModels.h"
#include "Framework/EidosPlayerController.h"
#include "Combat/WS_CombatDirector.h"
#include "World/Settlement/WS_Economy.h"
#include "World/Settlement/WS_Population.h"
#include "World/Settlement/WS_PortalDirector.h"
#include "Entities/Page/PageCharacter.h"
#include "Entities/Page/Components/StatsComponent.h"
#include "Kismet/GameplayStatics.h"

void UBaseHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindEconomy();
	RefreshResourcePanel();
	RefreshPageHUD();
}

void UBaseHUDWidget::NativeDestruct()
{
	UnbindEconomy();
	Super::NativeDestruct();
}

void UBaseHUDWidget::BindEconomy()
{
	if (!GetWorld()) return;

	Economy = GetWorld()->GetSubsystem<UWS_Economy>();
	if (!Economy) return;

	// ✅ Dynamic multicast delegate 가정
	Economy->OnEconomyChanged.AddDynamic(this, &UBaseHUDWidget::HandleEconomyChanged);
}

void UBaseHUDWidget::UnbindEconomy()
{
	if (!Economy) return;

	Economy->OnEconomyChanged.RemoveDynamic(this, &UBaseHUDWidget::HandleEconomyChanged);
	Economy = nullptr;
}

void UBaseHUDWidget::HandleEconomyChanged()
{
	RefreshResourcePanel();
}

void UBaseHUDWidget::RefreshResourcePanel()
{
	if (!ResourcePanel || !GetWorld()) return;

	UWS_Economy* Eco = Economy ? Economy : GetWorld()->GetSubsystem<UWS_Economy>();
	if (!Eco) return;
	
	const int32 Food  = Eco->GetAmount(FName("Food"));
	const int32 Wood  = Eco->GetAmount(FName("Wood"));
	const int32 Stone = Eco->GetAmount(FName("Stone"));
	const int32 Metal = Eco->GetAmount(FName("Metal"));
	const int32 EP = Eco->GetAmount(FName("EP"));

	ResourcePanel->SetResources(Food, Wood, Stone, Metal, EP);
}

void UBaseHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	HUDRefreshAccumulator += InDeltaTime;
	if (NotificationFeed)
	{
		NotificationFeed->TickFeed(InDeltaTime);
	}

	if (HUDRefreshAccumulator < 0.15f)
	{
		return;
	}

	HUDRefreshAccumulator = 0.f;
	RefreshPageHUD();
}

void UBaseHUDWidget::RefreshPageHUD()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UWS_Population* Population = World->GetSubsystem<UWS_Population>();
	if (!Population)
	{
		return;
	}

	AEidosPlayerController* EidosPC = Cast<AEidosPlayerController>(GetOwningPlayer());
	if (!EidosPC)
	{
		EidosPC = Cast<AEidosPlayerController>(UGameplayStatics::GetPlayerController(World, 0));
	}
	APageCharacter* SelectedPage = EidosPC ? EidosPC->GetSelectedPage() : nullptr;
	UWS_CombatDirector* CombatDirector = World->GetSubsystem<UWS_CombatDirector>();

	TArray<FPageSummaryView> PageViews;
	PageViews.Reserve(Population->GetOwnedPages().Num());

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

		PageViews.Add(View);
	}

	if (PageRoster)
	{
		PageRoster->SetPages(PageViews);
	}

	if (QuickBarPopup)
	{
		TArray<FPageQuickSlotView> QuickSlots;
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
				QuickSlots.Add(SlotView);
			}
		}
		QuickBarPopup->SetSlots(QuickSlots);
	}

	if (DungeonStatus)
	{
		FDungeonStatusView StatusView;
		StatusView.SelectedPageId = SelectedPage ? SelectedPage->GetPageEntityId() : INDEX_NONE;
		StatusView.bVisible = SelectedPage && SelectedPage->IsInDungeon();
		StatusView.Title = FText::FromString(TEXT("Dungeon Objective"));
		StatusView.Objective = FText::FromString(TEXT("Reach and destroy the dungeon core"));

		if (SelectedPage && CombatDirector && CombatDirector->IsCombatActive())
		{
			StatusView.Secondary = FText::FromString(TEXT("Turn combat active"));
		}
		else if (SelectedPage && SelectedPage->IsInDungeon())
		{
			StatusView.Secondary = FText::FromString(TEXT("Explore hostile settlement"));
		}

		if (UWS_PortalDirector* PortalDirector = World->GetSubsystem<UWS_PortalDirector>())
		{
			StatusView.ActivePortalCount = PortalDirector->GetActivePortals().Num();
		}

		DungeonStatus->SetDungeonStatus(StatusView);
	}
}
