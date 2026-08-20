#include "UI/HUD/Panels/PageWorkPriorityEditorWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Entities/Page/PageCharacter.h"
#include "UI/HUD/Panels/PageWorkPriorityRow.h"

void UPageWorkPriorityEditorWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!PriorityRowClass)
	{
		PriorityRowClass = LoadClass<UPageWorkPriorityRow>(nullptr,
			TEXT("/Game/Blueprints/WBP/WBP_PageWorkPriorityRow.WBP_PageWorkPriorityRow_C"));
	}
	if (Button_Close && !Button_Close->OnClicked.IsAlreadyBound(this, &UPageWorkPriorityEditorWidget::HandleCloseClicked))
	{
		Button_Close->OnClicked.AddDynamic(this, &UPageWorkPriorityEditorWidget::HandleCloseClicked);
	}
}

void UPageWorkPriorityEditorWidget::OpenForPage(APageCharacter* InPage)
{
	EditedPage = InPage;
	RefreshEditor();
}

void UPageWorkPriorityEditorWidget::CloseEditor()
{
	RemoveFromParent();
}

void UPageWorkPriorityEditorWidget::SetPriority(EWorkCategory WorkCategory, int32 NewPriority)
{
	if (APageCharacter* Page = EditedPage.Get())
	{
		Page->SetWorkPriority(WorkCategory, NewPriority);
		RefreshEditor();
	}
}

void UPageWorkPriorityEditorWidget::RefreshEditor()
{
	APageCharacter* Page = EditedPage.Get();
	if (!Page)
	{
		CloseEditor();
		return;
	}

	if (Text_PageName) Text_PageName->SetText(FText::FromString(GetNameSafe(Page)));
	if (Text_Instructions)
	{
		Text_Instructions->SetText(FText::FromString(TEXT("0 disables automatic assignment. Higher values are preferred when work is available.")));
	}
	if (!VerticalBox_PriorityRows || !PriorityRowClass) return;

	VerticalBox_PriorityRows->ClearChildren();
	for (int32 Index = 0; Index <= static_cast<int32>(EWorkCategory::Research); ++Index)
	{
		const EWorkCategory Category = static_cast<EWorkCategory>(Index);
		UPageWorkPriorityRow* Row = CreateWidget<UPageWorkPriorityRow>(this, PriorityRowClass);
		if (!Row) continue;
		Row->Setup(this, Category, Page->GetWorkPriority(Category));
		if (UVerticalBoxSlot* RowSlot = VerticalBox_PriorityRows->AddChildToVerticalBox(Row))
		{
			RowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
		}
	}
}

void UPageWorkPriorityEditorWidget::HandleCloseClicked()
{
	CloseEditor();
}
