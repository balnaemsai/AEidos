#include "UI/HUD/Panels/PageSkillEditorWidget.h"

#include "UI/HUD/Panels/Panel_Pages.h"
#include "UI/HUD/Panels/PageSkillEditorActionEntry.h"
#include "UI/HUD/Panels/PageQuickbarSlot.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

void UPageSkillEditorWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!QuickbarSlotClass)
	{
		QuickbarSlotClass = LoadClass<UPageQuickbarSlot>(nullptr,
			TEXT("/Game/Blueprints/WBP/WBP_PageQuickBarSlot.WBP_PageQuickBarSlot_C"));
	}
	if (!ActionEntryClass)
	{
		ActionEntryClass = LoadClass<UPageSkillEditorActionEntry>(nullptr,
			TEXT("/Game/Blueprints/WBP/WBP_PageSkillEditorActionEntry.WBP_PageSkillEditorActionEntry_C"));
	}
	if (Button_ClearSlot) Button_ClearSlot->OnClicked.AddDynamic(this, &UPageSkillEditorWidget::HandleClearSlotClicked);
	if (Button_Close) Button_Close->OnClicked.AddDynamic(this, &UPageSkillEditorWidget::HandleCloseClicked);
}

void UPageSkillEditorWidget::OpenForPanel(UPanel_Pages* InSourcePanel)
{
	SourcePanel = InSourcePanel;
	PendingAction = FPageActionCandidateView{};
	SelectedSlotIndex = 0;
	RefreshEditor();
}

void UPageSkillEditorWidget::CloseEditor()
{
	RemoveFromParent();
}

void UPageSkillEditorWidget::RefreshEditor()
{
	UPanel_Pages* Panel = SourcePanel.Get();
	if (!Panel)
	{
		CloseEditor();
		return;
	}

	Panel->RefreshFromWorld();
	if (Text_PageName) Text_PageName->SetText(Panel->GetSelectedPageSummary().DisplayName);
	RebuildActionEntries();
	RebuildSlotEntries();
	RefreshSelectionText();
}

void UPageSkillEditorWidget::RebuildActionEntries()
{
	if (!VerticalBox_AvailableActions)
	{
		return;
	}

	VerticalBox_AvailableActions->ClearChildren();
	if (!ActionEntryClass || !SourcePanel.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Pages] Cannot build action entries. Class=%s Source=%s"),
			*GetNameSafe(ActionEntryClass), *GetNameSafe(SourcePanel.Get()));
		return;
	}

	for (const FPageActionCandidateView& Action : SourcePanel->GetSelectedPageAvailableActions())
	{
		UPageSkillEditorActionEntry* Entry = CreateWidget<UPageSkillEditorActionEntry>(this, ActionEntryClass);
		if (!Entry) continue;

		const bool bSelected = PendingAction.ActionType == Action.ActionType && PendingAction.ActionId == Action.ActionId;
		Entry->Setup(Action, bSelected);
		Entry->OnActionClicked.AddDynamic(this, &UPageSkillEditorWidget::HandleActionClicked);
		if (UVerticalBoxSlot* EntrySlot = VerticalBox_AvailableActions->AddChildToVerticalBox(Entry))
		{
			EntrySlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
		}
	}
}

void UPageSkillEditorWidget::RebuildSlotEntries()
{
	if (!UniformGrid_QuickbarSlots)
	{
		return;
	}

	UniformGrid_QuickbarSlots->ClearChildren();
	if (!QuickbarSlotClass || !SourcePanel.IsValid())
	{
		return;
	}

	const int32 Columns = FMath::Max(1, QuickbarColumns);
	for (const FPageQuickSlotView& SlotView : SourcePanel->GetSelectedPageQuickSlots())
	{
		UPageQuickbarSlot* Entry = CreateWidget<UPageQuickbarSlot>(this, QuickbarSlotClass);
		if (!Entry) continue;

		Entry->Setup(SlotView);
		Entry->SetEditorSelected(SlotView.SlotIndex == SelectedSlotIndex);
		Entry->OnSlotClicked.AddDynamic(this, &UPageSkillEditorWidget::HandleSlotClicked);
		if (UUniformGridSlot* GridEntrySlot = UniformGrid_QuickbarSlots->AddChildToUniformGrid(Entry, SlotView.SlotIndex / Columns, SlotView.SlotIndex % Columns))
		{
			GridEntrySlot->SetHorizontalAlignment(HAlign_Fill);
			GridEntrySlot->SetVerticalAlignment(VAlign_Fill);
		}
	}
}

void UPageSkillEditorWidget::RefreshSelectionText()
{
	if (Text_SelectedSlot)
	{
		Text_SelectedSlot->SetText(SelectedSlotIndex == INDEX_NONE
			? FText::FromString(TEXT("SELECT A SLOT"))
			: FText::Format(FText::FromString(TEXT("SLOT {0}")), SelectedSlotIndex == 9 ? 0 : SelectedSlotIndex + 1));
	}
	if (Text_SelectedAction)
	{
		Text_SelectedAction->SetText(PendingAction.ActionType == EPageCombatActionType::None
			? FText::FromString(TEXT("SELECT AN ACTION"))
			: PendingAction.DisplayName);
	}
}

void UPageSkillEditorWidget::HandleActionClicked(FPageActionCandidateView Action)
{
	PendingAction = Action;
	RebuildActionEntries();
	RefreshSelectionText();
}

void UPageSkillEditorWidget::HandleSlotClicked(int32 SlotIndex)
{
	SelectedSlotIndex = SlotIndex;
	if (PendingAction.ActionType != EPageCombatActionType::None)
	{
		if (UPanel_Pages* Panel = SourcePanel.Get())
		{
			FPageCombatActionSlot SlotData;
			SlotData.ActionType = PendingAction.ActionType;
			SlotData.ActionId = PendingAction.ActionId;
			SlotData.DisplayName = PendingAction.DisplayName;
			Panel->AssignSelectedPageQuickSlot(SelectedSlotIndex, SlotData);
			PendingAction = FPageActionCandidateView{};
		}
	}
	RefreshEditor();
}

void UPageSkillEditorWidget::HandleClearSlotClicked()
{
	if (SelectedSlotIndex != INDEX_NONE)
	{
		if (UPanel_Pages* Panel = SourcePanel.Get())
		{
			Panel->ClearSelectedPageQuickSlot(SelectedSlotIndex);
		}
	}
	RefreshEditor();
}

void UPageSkillEditorWidget::HandleCloseClicked()
{
	CloseEditor();
}
