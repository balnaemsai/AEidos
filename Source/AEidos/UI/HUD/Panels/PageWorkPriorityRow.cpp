#include "UI/HUD/Panels/PageWorkPriorityRow.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "UI/HUD/Panels/PageWorkPriorityEditorWidget.h"

namespace
{
	FText GetCategoryText(EWorkCategory Category)
	{
		switch (Category)
		{
		case EWorkCategory::Craft: return FText::FromString(TEXT("Craft"));
		case EWorkCategory::Gather: return FText::FromString(TEXT("Gather"));
		case EWorkCategory::Construction: return FText::FromString(TEXT("Construction"));
		case EWorkCategory::Research: return FText::FromString(TEXT("Research"));
		default: return FText::GetEmpty();
		}
	}
}

void UPageWorkPriorityRow::NativeConstruct()
{
	Super::NativeConstruct();
	if (Button_Decrease && !Button_Decrease->OnClicked.IsAlreadyBound(this, &UPageWorkPriorityRow::HandleDecreaseClicked))
	{
		Button_Decrease->OnClicked.AddDynamic(this, &UPageWorkPriorityRow::HandleDecreaseClicked);
	}
	if (Button_Increase && !Button_Increase->OnClicked.IsAlreadyBound(this, &UPageWorkPriorityRow::HandleIncreaseClicked))
	{
		Button_Increase->OnClicked.AddDynamic(this, &UPageWorkPriorityRow::HandleIncreaseClicked);
	}
	RefreshText();
}

void UPageWorkPriorityRow::Setup(UPageWorkPriorityEditorWidget* InEditor, EWorkCategory InCategory, int32 InPriority)
{
	Editor = InEditor;
	WorkCategory = InCategory;
	Priority = FMath::Clamp(InPriority, 0, 5);
	RefreshText();
}

void UPageWorkPriorityRow::RefreshText()
{
	if (Text_Category) Text_Category->SetText(GetCategoryText(WorkCategory));
	if (Text_Priority) Text_Priority->SetText(FText::AsNumber(Priority));
	if (Button_Decrease) Button_Decrease->SetIsEnabled(Priority > 0);
	if (Button_Increase) Button_Increase->SetIsEnabled(Priority < 5);
}

void UPageWorkPriorityRow::HandleDecreaseClicked()
{
	if (UPageWorkPriorityEditorWidget* Owner = Editor.Get())
	{
		Owner->SetPriority(WorkCategory, Priority - 1);
	}
}

void UPageWorkPriorityRow::HandleIncreaseClicked()
{
	if (UPageWorkPriorityEditorWidget* Owner = Editor.Get())
	{
		Owner->SetPriority(WorkCategory, Priority + 1);
	}
}
