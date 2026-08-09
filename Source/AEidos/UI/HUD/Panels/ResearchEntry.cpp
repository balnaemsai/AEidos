#include "UI/HUD/Panels/ResearchEntry.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

void UResearchEntry::NativeConstruct()
{
	Super::NativeConstruct();
	if (Button_Start) Button_Start->OnClicked.AddDynamic(this, &UResearchEntry::HandleStartClicked);
	if (Button_Cancel) Button_Cancel->OnClicked.AddDynamic(this, &UResearchEntry::HandleCancelClicked);
}

void UResearchEntry::Setup(const FResearchView& InView)
{
	ResearchId = InView.ResearchId;
	if (Text_Name) Text_Name->SetText(InView.DisplayName);
	if (Text_Description) Text_Description->SetText(InView.Description);
	if (Text_Prerequisites)
	{
		FString Requirements;
		for (const FName Prerequisite : InView.PrerequisiteResearchIds)
		{
			if (!Requirements.IsEmpty()) Requirements += TEXT(", ");
			Requirements += Prerequisite.ToString();
		}
		Text_Prerequisites->SetText(Requirements.IsEmpty() ? FText::FromString(TEXT("No prerequisite")) : FText::Format(FText::FromString(TEXT("Requires: {0}")), FText::FromString(Requirements)));
	}

	FString Status;
	if (InView.bCompleted) Status = TEXT("COMPLETED");
	else if (InView.bCanCancel) Status = FString::Printf(TEXT("IN PROGRESS  %.0f / %.0f  |  Workers %d/%d"), InView.Progress, InView.TotalWork, InView.ActiveWorkers, InView.MaxWorkers);
	else if (!InView.bPrerequisitesMet) Status = TEXT("LOCKED: prerequisite research required");
	else Status = TEXT("AVAILABLE");
	if (Text_Status) Text_Status->SetText(FText::FromString(Status));
	if (Button_Start) { Button_Start->SetIsEnabled(InView.bCanStart); Button_Start->SetVisibility(InView.bCompleted ? ESlateVisibility::Collapsed : ESlateVisibility::Visible); }
	if (Button_Cancel) { Button_Cancel->SetIsEnabled(InView.bCanCancel); Button_Cancel->SetVisibility(InView.bCanCancel ? ESlateVisibility::Visible : ESlateVisibility::Collapsed); }
}

void UResearchEntry::HandleStartClicked() { if (!ResearchId.IsNone()) OnResearchStartRequested.Broadcast(ResearchId); }
void UResearchEntry::HandleCancelClicked() { if (!ResearchId.IsNone()) OnResearchCancelRequested.Broadcast(ResearchId); }
