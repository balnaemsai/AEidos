#include "Progression/WS_Scenario.h"

#include "Data/Definitions/ScenarioDefinitionRow.h"
#include "Data/GIS_DataRegistry.h"
#include "Engine/GameInstance.h"
#include "Framework/EidosGameMode.h"
#include "World/Dungeon/WS_DungeonRuntime.h"
#include "World/Settlement/WS_Building.h"
#include "World/Settlement/WS_Population.h"
#include "World/Settlement/WS_Research.h"

void UWS_Scenario::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	DungeonRuntime = GetWorld() ? GetWorld()->GetSubsystem<UWS_DungeonRuntime>() : nullptr;
	if (DungeonRuntime.IsValid())
	{
		DungeonCoreDestroyedHandle = DungeonRuntime->OnDungeonCoreDestroyedForScenario.AddUObject(this, &UWS_Scenario::HandleDungeonCoreDestroyed);
	}
}

void UWS_Scenario::Deinitialize()
{
	if (DungeonRuntime.IsValid() && DungeonCoreDestroyedHandle.IsValid())
	{
		DungeonRuntime->OnDungeonCoreDestroyedForScenario.Remove(DungeonCoreDestroyedHandle);
	}
	Super::Deinitialize();
}

void UWS_Scenario::SimPlan_Implementation(USimCommandBuffer* CommandBuffer, float FixedDeltaSeconds)
{
	if (ActiveScenarioId.IsNone() || bCompleted)
	{
		return;
	}

	const FScenarioDefinitionRow* Definition = FindActiveDefinition();
	if (!Definition || !AreRequirementsMet(*Definition))
	{
		return;
	}

	bCompleted = true;
	OnScenarioStateChanged.Broadcast();
	if (AEidosGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEidosGameMode>() : nullptr)
	{
		GameMode->TriggerGameVictory(Definition->DisplayName, Definition->Description);
	}
}

void UWS_Scenario::WriteToSnapshot_Implementation(FEidosWorldSnapshot& InOutSnapshot) const
{
	InOutSnapshot.Scenario.ActiveScenarioId = ActiveScenarioId;
	InOutSnapshot.Scenario.DestroyedDungeonCoreCount = DestroyedDungeonCoreCount;
	InOutSnapshot.Scenario.bCompleted = bCompleted;
}

void UWS_Scenario::ApplySnapshot_Implementation(const FEidosWorldSnapshot& Snapshot)
{
	ActiveScenarioId = Snapshot.Scenario.ActiveScenarioId;
	DestroyedDungeonCoreCount = FMath::Max(0, Snapshot.Scenario.DestroyedDungeonCoreCount);
	bCompleted = Snapshot.Scenario.bCompleted;
	if (bCompleted)
	{
		if (const FScenarioDefinitionRow* Definition = FindActiveDefinition())
		{
			if (AEidosGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEidosGameMode>() : nullptr)
			{
				GameMode->TriggerGameVictory(Definition->DisplayName, Definition->Description);
			}
		}
	}
	OnScenarioStateChanged.Broadcast();
}

bool UWS_Scenario::SelectScenario(FName ScenarioId)
{
	if (ScenarioId.IsNone() || !GetWorld())
	{
		return false;
	}

	UGameInstance* GameInstance = GetWorld()->GetGameInstance();
	UGIS_DataRegistry* Registry = GameInstance ? GameInstance->GetSubsystem<UGIS_DataRegistry>() : nullptr;
	if (!Registry || !Registry->EnsureReadySync() || !Registry->GetScenarioDef(ScenarioId))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Scenario] Select failed. Unknown ScenarioId=%s"), *ScenarioId.ToString());
		return false;
	}

	ActiveScenarioId = ScenarioId;
	DestroyedDungeonCoreCount = 0;
	bCompleted = false;
	OnScenarioStateChanged.Broadcast();
	UE_LOG(LogTemp, Log, TEXT("[Scenario] Selected ScenarioId=%s"), *ScenarioId.ToString());
	return true;
}

const FScenarioDefinitionRow* UWS_Scenario::FindActiveDefinition() const
{
	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UGIS_DataRegistry* Registry = GameInstance ? GameInstance->GetSubsystem<UGIS_DataRegistry>() : nullptr;
	return Registry && Registry->EnsureReadySync() ? Registry->GetScenarioDef(ActiveScenarioId) : nullptr;
}

bool UWS_Scenario::AreRequirementsMet(const FScenarioDefinitionRow& Definition) const
{
	const bool bHasRequirement = !Definition.RequiredResearchIds.IsEmpty()
		|| !Definition.RequiredBuildingIds.IsEmpty()
		|| Definition.RequiredDungeonCoreDestructions > 0
		|| Definition.RequiredPageCount > 0;
	if (!bHasRequirement)
	{
		return false;
	}

	if (Definition.RequiredDungeonCoreDestructions > DestroyedDungeonCoreCount)
	{
		return false;
	}

	if (Definition.RequiredPageCount > 0)
	{
		const UWS_Population* Population = GetWorld()->GetSubsystem<UWS_Population>();
		if (!Population || Population->GetCurrentPageCount() < Definition.RequiredPageCount) return false;
	}

	if (!Definition.RequiredResearchIds.IsEmpty())
	{
		const UWS_Research* Research = GetWorld()->GetSubsystem<UWS_Research>();
		if (!Research || !Research->HasAllCompletedResearch(Definition.RequiredResearchIds)) return false;
	}

	if (!Definition.RequiredBuildingIds.IsEmpty())
	{
		const UWS_Building* Buildings = GetWorld()->GetSubsystem<UWS_Building>();
		if (!Buildings) return false;
		TArray<FName> CompletedBuildingIds;
		Buildings->GetCompletedBuildingIds(CompletedBuildingIds);
		for (const FName BuildingId : Definition.RequiredBuildingIds)
		{
			if (!CompletedBuildingIds.Contains(BuildingId)) return false;
		}
	}

	return true;
}

void UWS_Scenario::HandleDungeonCoreDestroyed(int32 PortalId)
{
	++DestroyedDungeonCoreCount;
	OnScenarioStateChanged.Broadcast();
	UE_LOG(LogTemp, Log, TEXT("[Scenario] Dungeon core destroyed. Total=%d PortalId=%d"), DestroyedDungeonCoreCount, PortalId);
}
