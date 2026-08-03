#include "UI/HUD/Panels/PageSkillEditorActionEntry.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void UPageSkillEditorActionEntry::NativeConstruct()
{
	Super::NativeConstruct();
	if (Button_Root)
	{
		Button_Root->OnClicked.AddDynamic(this, &UPageSkillEditorActionEntry::HandleClicked);
	}
}

void UPageSkillEditorActionEntry::Setup(const FPageActionCandidateView& InAction, bool bSelected)
{
	ActionData = InAction;
	if (Text_ActionName) Text_ActionName->SetText(ActionData.DisplayName);
	if (Text_APCost) Text_APCost->SetText(FText::Format(FText::FromString(TEXT("AP {0}")), ActionData.ActionPointCost));
	if (Text_Assignment) Text_Assignment->SetText(ActionData.bAssignedToQuickBar ? FText::FromString(TEXT("ASSIGNED")) : FText::GetEmpty());
	if (Border_Background)
	{
		Border_Background->SetBrushColor(bSelected
			? FLinearColor(0.24f, 0.23f, 0.20f, 0.92f)
			: FLinearColor(0.08f, 0.10f, 0.11f, 0.86f));
	}
}

void UPageSkillEditorActionEntry::HandleClicked()
{
	OnActionClicked.Broadcast(ActionData);
}
