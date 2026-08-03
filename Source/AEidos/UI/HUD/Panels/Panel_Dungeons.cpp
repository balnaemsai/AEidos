#include "UI/HUD/Panels/Panel_Dungeons.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Entities/Page/PageCharacter.h"
#include "Framework/EidosPlayerController.h"
#include "TimerManager.h"
#include "UI/HUD/Panels/DungeonEntry.h"
#include "World/Settlement/WS_PortalDirector.h"

namespace
{
	FLinearColor UiColor(const TCHAR* Hex, float Alpha = 1.f)
	{
		FLinearColor Color = FLinearColor::FromSRGBColor(FColor::FromHex(Hex));
		Color.A = Alpha;
		return Color;
	}

	FText BuildPortalStatusText(const FPortalState& Portal)
	{
		if (Portal.bCleared || Portal.Status == EPortalStatus::Cleared)
		{
			return FText::FromString(TEXT("클리어됨"));
		}

		switch (Portal.Status)
		{
		case EPortalStatus::Spawning: return FText::FromString(TEXT("생성 중"));
		case EPortalStatus::Available: return FText::FromString(TEXT("진입 가능"));
		case EPortalStatus::Entered: return FText::FromString(TEXT("원정 진행 중"));
		case EPortalStatus::RaidTriggered: return FText::FromString(TEXT("레이드 발생"));
		case EPortalStatus::Expired: return FText::FromString(TEXT("정리 중"));
		default: return FText::GetEmpty();
		}
	}

	FLinearColor BuildPortalStatusColor(const FPortalState& Portal)
	{
		if (Portal.bCleared || Portal.Status == EPortalStatus::Cleared)
		{
			return UiColor(TEXT("859188"), 0.85f);
		}

		switch (Portal.Status)
		{
		case EPortalStatus::Available: return UiColor(TEXT("AAB0B2"), 0.9f);
		case EPortalStatus::Entered: return UiColor(TEXT("B8B1A2"), 0.9f);
		case EPortalStatus::RaidTriggered: return UiColor(TEXT("9C8380"), 0.9f);
		case EPortalStatus::Spawning: return UiColor(TEXT("6E7478"), 0.9f);
		default: return UiColor(TEXT("70767A"), 0.85f);
		}
	}

	FText FormatRaidTimer(float Seconds)
	{
		const int32 TotalSeconds = FMath::Max(0, FMath::CeilToInt(Seconds));
		return FText::FromString(FString::Printf(TEXT("레이드까지 %02d:%02d"), TotalSeconds / 60, TotalSeconds % 60));
	}
}

void UPanel_Dungeons::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_FocusPortal)
	{
		Button_FocusPortal->OnClicked.AddDynamic(this, &UPanel_Dungeons::HandleFocusPortalClicked);
	}

	RefreshFromWorld();
}

void UPanel_Dungeons::NativeDestruct()
{
	StopAutoRefresh();
	Super::NativeDestruct();
}

void UPanel_Dungeons::OnPanelShown_Implementation()
{
	RefreshFromWorld();
	StartAutoRefresh();
}

void UPanel_Dungeons::OnPanelHidden_Implementation()
{
	StopAutoRefresh();
}

void UPanel_Dungeons::RefreshFromWorld()
{
	CachedPortals.Reset();
	CachedPortalViews.Reset();
	SelectedPortalView = FDungeonPortalView{};
	bSelectedPageInDungeon = false;

	UWorld* World = GetWorld();
	if (!World)
	{
		RefreshDetailWidgets();
		return;
	}

	APageCharacter* SelectedPage = nullptr;
	if (AEidosPlayerController* EidosPC = Cast<AEidosPlayerController>(GetOwningPlayer()))
	{
		SelectedPage = EidosPC->GetSelectedPage();
		bSelectedPageInDungeon = SelectedPage && SelectedPage->IsInDungeon();
	}

	if (UWS_PortalDirector* PortalDirector = World->GetSubsystem<UWS_PortalDirector>())
	{
		CachedPortals = PortalDirector->GetActivePortals();
	}

	CachedPortals.Sort([](const FPortalState& A, const FPortalState& B)
	{
		return A.PortalId < B.PortalId;
	});

	for (const FPortalState& Portal : CachedPortals)
	{
		FDungeonPortalView View;
		View.PortalId = Portal.PortalId;
		View.DisplayName = FText::FromString(Portal.PortalDefId.IsNone() ? TEXT("이름 없는 포탈") : Portal.PortalDefId.ToString());
		View.TierText = FText::Format(FText::FromString(TEXT("Tier {0}")), Portal.Tier);
		View.StatusText = BuildPortalStatusText(Portal);
		View.RaidTimerText = FormatRaidTimer(Portal.RaidTimer);
		View.StatusColor = BuildPortalStatusColor(Portal);
		View.bDungeonEntered = Portal.bDungeonEntered || Portal.Status == EPortalStatus::Entered;
		View.bCleared = Portal.bCleared || Portal.Status == EPortalStatus::Cleared;
		View.ExpeditionText = View.bCleared
			? FText::FromString(TEXT("던전 클리어"))
			: View.bDungeonEntered
				? FText::FromString(TEXT("원정 진행 중"))
				: FText::FromString(TEXT("원정 대기"));

		if (SelectedPage && !bSelectedPageInDungeon)
		{
			const int32 DistanceMeters = FMath::RoundToInt(FVector::Distance(SelectedPage->GetActorLocation(), Portal.Location) / 100.f);
			View.DistanceText = FText::Format(FText::FromString(TEXT("거리 {0}m")), DistanceMeters);
		}
		else if (bSelectedPageInDungeon)
		{
			View.DistanceText = FText::FromString(TEXT("선택 Page는 던전 내"));
		}
		else
		{
			View.DistanceText = FText::GetEmpty();
		}

		CachedPortalViews.Add(View);
	}

	if (!CachedPortalViews.ContainsByPredicate([this](const FDungeonPortalView& View) { return View.PortalId == SelectedPortalId; }))
	{
		SelectedPortalId = CachedPortalViews.IsEmpty() ? INDEX_NONE : CachedPortalViews[0].PortalId;
	}

	for (FDungeonPortalView& View : CachedPortalViews)
	{
		View.bIsSelected = View.PortalId == SelectedPortalId;
		if (View.bIsSelected)
		{
			SelectedPortalView = View;
		}
	}

	RebuildPortalEntryWidgets();
	RefreshDetailWidgets();
}

bool UPanel_Dungeons::RequestSelectPortal(int32 PortalId)
{
	if (!CachedPortalViews.ContainsByPredicate([PortalId](const FDungeonPortalView& View) { return View.PortalId == PortalId; }))
	{
		return false;
	}

	SelectedPortalId = PortalId;
	RefreshFromWorld();
	return true;
}

void UPanel_Dungeons::RebuildPortalEntryWidgets()
{
	// Some UMG widgets are not marked as Blueprint variables. Resolve by name as
	// a fallback so the list still works as long as the required widget name exists.
	if (!VerticalBox_PortalEntries)
	{
		VerticalBox_PortalEntries = Cast<UVerticalBox>(GetWidgetFromName(TEXT("VerticalBox_PortalEntries")));
	}

	if (!VerticalBox_PortalEntries)
	{
		if (!bHasLoggedEntrySetupFailure)
		{
			UE_LOG(LogTemp, Warning, TEXT("[DungeonPanel] Missing VerticalBox_PortalEntries in %s"), *GetNameSafe(this));
			bHasLoggedEntrySetupFailure = true;
		}
		return;
	}

	VerticalBox_PortalEntries->ClearChildren();
	if (!DungeonEntryClass)
	{
		if (!bHasLoggedEntrySetupFailure)
		{
			UE_LOG(LogTemp, Warning, TEXT("[DungeonPanel] DungeonEntryClass is not assigned in %s"), *GetNameSafe(this));
			bHasLoggedEntrySetupFailure = true;
		}
		return;
	}

	int32 CreatedEntryCount = 0;
	for (const FDungeonPortalView& PortalView : CachedPortalViews)
	{
		UDungeonEntry* Entry = CreateWidget<UDungeonEntry>(this, DungeonEntryClass);
		if (!Entry)
		{
			continue;
		}

		Entry->Setup(PortalView);
		Entry->OnEntryClicked.AddDynamic(this, &UPanel_Dungeons::HandleDungeonEntryClicked);
		if (UVerticalBoxSlot* EntrySlot = VerticalBox_PortalEntries->AddChildToVerticalBox(Entry))
		{
			EntrySlot->SetHorizontalAlignment(HAlign_Fill);
			EntrySlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
		}
		++CreatedEntryCount;
	}

	if (CachedPortalViews.Num() > 0 && CreatedEntryCount == 0 && !bHasLoggedEntrySetupFailure)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DungeonPanel] Failed to create entries. Class=%s PortalCount=%d"), *GetNameSafe(DungeonEntryClass), CachedPortalViews.Num());
		bHasLoggedEntrySetupFailure = true;
	}
}

void UPanel_Dungeons::RefreshDetailWidgets()
{
	const bool bHasSelection = SelectedPortalView.PortalId != INDEX_NONE;

	if (Text_SelectedPortalName)
	{
		Text_SelectedPortalName->SetText(bHasSelection ? SelectedPortalView.DisplayName : FText::FromString(TEXT("활성 포탈 없음")));
	}
	if (Text_SelectedTier)
	{
		Text_SelectedTier->SetText(bHasSelection ? SelectedPortalView.TierText : FText::GetEmpty());
	}
	if (Text_PortalStatus)
	{
		Text_PortalStatus->SetText(bHasSelection ? SelectedPortalView.StatusText : FText::GetEmpty());
	}
	if (Text_RaidTimer)
	{
		Text_RaidTimer->SetText(bHasSelection ? SelectedPortalView.RaidTimerText : FText::GetEmpty());
	}
	if (Text_ExpeditionStatus)
	{
		Text_ExpeditionStatus->SetText(bHasSelection ? SelectedPortalView.ExpeditionText : FText::GetEmpty());
	}
	if (Text_PortalDistance)
	{
		Text_PortalDistance->SetText(bHasSelection ? SelectedPortalView.DistanceText : FText::GetEmpty());
	}
	if (Text_EmptyState)
	{
		Text_EmptyState->SetVisibility(CachedPortalViews.IsEmpty() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (Button_FocusPortal)
	{
		Button_FocusPortal->SetIsEnabled(bHasSelection);
	}
}

void UPanel_Dungeons::StartAutoRefresh()
{
	if (UWorld* World = GetWorld())
	{
		if (!World->GetTimerManager().IsTimerActive(RefreshTimerHandle))
		{
			World->GetTimerManager().SetTimer(RefreshTimerHandle, this, &UPanel_Dungeons::RefreshFromWorld, RefreshIntervalSeconds, true);
		}
	}
}

void UPanel_Dungeons::StopAutoRefresh()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RefreshTimerHandle);
	}
}

void UPanel_Dungeons::HandleDungeonEntryClicked(int32 PortalId)
{
	RequestSelectPortal(PortalId);
}

void UPanel_Dungeons::HandleFocusPortalClicked()
{
	if (SelectedPortalId != INDEX_NONE)
	{
		OnFocusPortalRequested(SelectedPortalId);
	}
}
