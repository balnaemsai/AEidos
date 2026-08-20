#include "World/Settlement/WS_Research.h"

#include "Data/GIS_DataRegistry.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "World/Settlement/WS_Work.h"

void UWS_Research::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UWS_Work>();
	WorkSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UWS_Work>() : nullptr;
	if (UWS_Work* Work = WorkSubsystem.Get())
	{
		WorkStateChangedHandle = Work->OnWorkRequestStateChanged.AddUObject(this, &UWS_Research::HandleWorkRequestStateChanged);
		WorkCompletedHandle = Work->OnWorkCompleted.AddUObject(this, &UWS_Research::HandleWorkCompleted);
	}
}

void UWS_Research::Deinitialize()
{
	if (UWS_Work* Work = WorkSubsystem.Get())
	{
		Work->OnWorkRequestStateChanged.Remove(WorkStateChangedHandle);
		Work->OnWorkCompleted.Remove(WorkCompletedHandle);
	}
	WorkStateChangedHandle.Reset();
	WorkCompletedHandle.Reset();
	WorkSubsystem.Reset();
	Super::Deinitialize();
}

void UWS_Research::LoadResearchDefs()
{
	ResearchDefs.Reset();
	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UGIS_DataRegistry* Registry = GameInstance ? GameInstance->GetSubsystem<UGIS_DataRegistry>() : nullptr;
	UDataTable* Table = Registry ? Registry->FindDataTableByName(TEXT("DT_Research")) : nullptr;
	if (!Table)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Research] DT_Research not found in DataRegistry."));
		return;
	}

	TArray<FResearchDefinitionRow*> Rows;
	Table->GetAllRows(TEXT("WS_Research::LoadResearchDefs"), Rows);
	for (const FResearchDefinitionRow* Row : Rows)
	{
		if (Row && !Row->ResearchId.IsNone() && !Row->ResearchWorkId.IsNone())
		{
			ResearchDefs.Add(Row->ResearchId, *Row);
		}
	}
	OnResearchChanged.Broadcast();
}

bool UWS_Research::HasCompletedResearch(FName ResearchId) const
{
	return !ResearchId.IsNone() && CompletedResearchIds.Contains(ResearchId);
}

bool UWS_Research::HasAllCompletedResearch(const TArray<FName>& ResearchIds) const
{
	return ResearchIds.ContainsByPredicate([this](FName ResearchId) { return !HasCompletedResearch(ResearchId); }) == false;
}

TArray<FResearchView> UWS_Research::GetResearchViews() const
{
	TArray<FResearchView> Views;
	TArray<FName> ResearchIds;
	ResearchDefs.GetKeys(ResearchIds);
	ResearchIds.Sort(FNameLexicalLess());
	for (FName ResearchId : ResearchIds)
	{
		const FResearchDefinitionRow* Definition = FindDefinition(ResearchId);
		if (!Definition)
		{
			continue;
		}

		FResearchView View;
		View.ResearchId = Definition->ResearchId;
		View.DisplayName = Definition->DisplayName;
		View.Description = Definition->Description;
		View.PrerequisiteResearchIds = Definition->PrerequisiteResearchIds;
		View.bCompleted = HasCompletedResearch(Definition->ResearchId);
		View.bPrerequisitesMet = ArePrerequisitesMet(*Definition);
		View.ActiveRequestId = FindOutstandingRequestId(*Definition);
		View.bCanCancel = View.ActiveRequestId != INDEX_NONE;
		View.bCanStart = !View.bCompleted && View.bPrerequisitesMet && !View.bCanCancel;

		if (UWS_Work* Work = WorkSubsystem.Get())
		{
			FWorkOrderView WorkView;
			if (Work->GetWorkOrderView(Definition->ResearchWorkId, WorkView))
			{
				View.ActiveWorkers = WorkView.ActiveWorkerCount;
				View.MaxWorkers = WorkView.ActiveMaxWorkers;
				View.Progress = WorkView.ActiveProgress;
				View.TotalWork = WorkView.ActiveTotalWork;
			}
		}
		Views.Add(MoveTemp(View));
	}
	return Views;
}

int32 UWS_Research::StartResearch(FName ResearchId)
{
	const FResearchDefinitionRow* Definition = FindDefinition(ResearchId);
	UWS_Work* Work = WorkSubsystem.Get();
	if (!Definition || !Work || HasCompletedResearch(ResearchId) || !ArePrerequisitesMet(*Definition)
		|| FindOutstandingRequestId(*Definition) != INDEX_NONE)
	{
		return INDEX_NONE;
	}

	const int32 RequestId = Work->QueueWorkById(Definition->ResearchWorkId, 1, 50);
	if (RequestId != INDEX_NONE)
	{
		UE_LOG(LogTemp, Log, TEXT("[Research] Started ResearchId=%s RequestId=%d"), *ResearchId.ToString(), RequestId);
		OnResearchChanged.Broadcast();
	}
	return RequestId;
}

bool UWS_Research::CancelResearch(FName ResearchId)
{
	const FResearchDefinitionRow* Definition = FindDefinition(ResearchId);
	UWS_Work* Work = WorkSubsystem.Get();
	const int32 RequestId = Definition ? FindOutstandingRequestId(*Definition) : INDEX_NONE;
	return Work && RequestId != INDEX_NONE && Work->CancelWorkRequest(RequestId);
}

void UWS_Research::WriteToSnapshot_Implementation(FEidosWorldSnapshot& InOutSnapshot) const
{
	InOutSnapshot.Research.CompletedResearchIds = CompletedResearchIds.Array();
}

void UWS_Research::ApplySnapshot_Implementation(const FEidosWorldSnapshot& Snapshot)
{
	CompletedResearchIds.Reset();
	for (FName ResearchId : Snapshot.Research.CompletedResearchIds)
	{
		if (!ResearchId.IsNone())
		{
			CompletedResearchIds.Add(ResearchId);
		}
	}
	OnResearchChanged.Broadcast();
}

const FResearchDefinitionRow* UWS_Research::FindDefinition(FName ResearchId) const
{
	return ResearchDefs.Find(ResearchId);
}

const FResearchDefinitionRow* UWS_Research::FindDefinitionByWorkId(FName WorkId) const
{
	for (const TPair<FName, FResearchDefinitionRow>& Pair : ResearchDefs)
	{
		if (Pair.Value.ResearchWorkId == WorkId)
		{
			return &Pair.Value;
		}
	}
	return nullptr;
}

bool UWS_Research::ArePrerequisitesMet(const FResearchDefinitionRow& Definition) const
{
	return HasAllCompletedResearch(Definition.PrerequisiteResearchIds);
}

int32 UWS_Research::FindOutstandingRequestId(const FResearchDefinitionRow& Definition) const
{
	UWS_Work* Work = WorkSubsystem.Get();
	if (!Work)
	{
		return INDEX_NONE;
	}
	TArray<int32> RequestIds;
	Work->GetOutstandingRequestIdsForWork(Definition.ResearchWorkId, RequestIds);
	return RequestIds.IsEmpty() ? INDEX_NONE : RequestIds.Last();
}

void UWS_Research::HandleWorkRequestStateChanged(int32 RequestId, EWorkRequestLifecycleState NewState)
{
	OnResearchChanged.Broadcast();
}

void UWS_Research::HandleWorkCompleted(int32 RequestId, FName WorkId)
{
	const FResearchDefinitionRow* Definition = FindDefinitionByWorkId(WorkId);
	if (!Definition || CompletedResearchIds.Contains(Definition->ResearchId))
	{
		return;
	}
	CompletedResearchIds.Add(Definition->ResearchId);
	UE_LOG(LogTemp, Log, TEXT("[Research] Completed ResearchId=%s RequestId=%d"), *Definition->ResearchId.ToString(), RequestId);
	OnResearchChanged.Broadcast();
}
