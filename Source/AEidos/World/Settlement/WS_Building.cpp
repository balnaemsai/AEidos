// Fill out your copyright notice in the Description page of Project Settings.


#include "World/Settlement/WS_Building.h"

#include "Data/GIS_DataRegistry.h"
#include "Data/Definitions/BuildingDefinitionRow.h"
#include "Data/Definitions/WorkDefinitionRow.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Simulation/SimCommandBuffer.h"
#include "World/Settlement/EidosAccessInterface.h"
#include "World/Settlement/WS_Economy.h"
#include "World/Settlement/WS_Work.h"
#include "World/Settlement/Building/BuildingActorBase.h"
#include "World/Settlement/Building/ConstructionSiteActor.h"

void UWS_Building::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Collection.InitializeDependency<UWS_Work>();
	WorkSubsystem = GetWorld()->GetSubsystem<UWS_Work>();
	if (WorkSubsystem)
	{
		WorkSubsystem->OnWorkRequestStateChanged.AddUObject(this, &UWS_Building::HandleWorkRequestStateChanged);
	}
}

void UWS_Building::Deinitialize()
{
	if (WorkSubsystem)
	{
		WorkSubsystem->OnWorkRequestStateChanged.RemoveAll(this);
	}

	DestroyAllRuntimeActors();
	Super::Deinitialize();
}

void UWS_Building::LoadBuildingDefs()
{
	BuildingDefs.Reset();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UGameInstance* GI = World->GetGameInstance();
	if (!GI)
	{
		return;
	}

	UGIS_DataRegistry* Registry = GI->GetSubsystem<UGIS_DataRegistry>();
	if (!Registry)
	{
		return;
	}

	UDataTable* Table = Registry->FindDataTableByName(TEXT("DT_Building"));
	if (!Table)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Building] DT_Building not found"));
		return;
	}

	TArray<FBuildingDefinitionRow*> Rows;
	Table->GetAllRows(TEXT("WS_Building::LoadBuildingDefs"), Rows);
	for (const FBuildingDefinitionRow* Row : Rows)
	{
		if (Row)
		{
			BuildingDefs.Add(Row->BuildingId, *Row);
		}
	}
}

const FBuildingDefinitionRow* UWS_Building::FindBuildingDef(FName BuildingId) const
{
	return BuildingDefs.Find(BuildingId);
}

FConstructionSiteState* UWS_Building::FindConstructionSiteById(int32 SiteId)
{
	return ConstructionSites.FindByPredicate([&](const FConstructionSiteState& Site)
	{
		return Site.SiteId == SiteId;
	});
}

FConstructionSiteState* UWS_Building::FindConstructionSiteByRequestId(int32 WorkRequestId)
{
	return ConstructionSites.FindByPredicate([&](const FConstructionSiteState& Site)
	{
		return Site.WorkRequestId == WorkRequestId;
	});
}

bool UWS_Building::IntersectsAnyPlacedOrConstruction(const FBuildingDefinitionRow& Def, FVector Location) const
{
	const FVector2D Half = Def.Footprint * 0.5f;
	const FVector2D Center(Location.X, Location.Y);

	auto Overlap2D = [](const FVector2D& ACenter, const FVector2D& AHalf, const FVector2D& BCenter, const FVector2D& BHalf)
	{
		return FMath::Abs(ACenter.X - BCenter.X) <= (AHalf.X + BHalf.X)
			&& FMath::Abs(ACenter.Y - BCenter.Y) <= (AHalf.Y + BHalf.Y);
	};

	for (const FConstructionSiteState& Site : ConstructionSites)
	{
		if (Site.State == EConstructionSiteLifecycle::Cancelled || Site.State == EConstructionSiteLifecycle::Failed)
		{
			continue;
		}

		const FBuildingDefinitionRow* OtherDef = FindBuildingDef(Site.BuildingId);
		if (!OtherDef)
		{
			continue;
		}

		const FVector2D OtherCenter(Site.Location.X, Site.Location.Y);
		const FVector2D OtherHalf = OtherDef->Footprint * 0.5f;
		if (Overlap2D(Center, Half, OtherCenter, OtherHalf))
		{
			return true;
		}
	}

	return false;
}

bool UWS_Building::ValidatePlacement(FName BuildingId, FVector Location, float YawDeg, FString& OutReason) const
{
	const FBuildingDefinitionRow* Def = FindBuildingDef(BuildingId);
	if (!Def)
	{
		OutReason = TEXT("Invalid BuildingId");
		return false;
	}

	if (IntersectsAnyPlacedOrConstruction(*Def, Location))
	{
		OutReason = TEXT("Placement overlaps existing building/site");
		return false;
	}

	OutReason.Empty();
	return true;
}

int32 UWS_Building::CreateConstructionSite(FName BuildingId, FVector Location, float YawDeg, int32 WorkRequestId)
{
	FConstructionSiteState Site;
	Site.SiteId = NextSiteId++;
	Site.BuildingId = BuildingId;
	Site.Location = Location;
	Site.YawDeg = YawDeg;
	Site.WorkRequestId = WorkRequestId;
	Site.State = EConstructionSiteLifecycle::Queued;
	ConstructionSites.Add(Site);

	FConstructionSiteState* AddedSite = FindConstructionSiteById(Site.SiteId);
	if (AddedSite)
	{
		SpawnConstructionSiteActor(*AddedSite);
	}

	return Site.SiteId;
}

int32 UWS_Building::RequestBuild(FName BuildingId, FVector Location, float YawDeg)
{
	if (!WorkSubsystem)
	{
		return INDEX_NONE;
	}

	UWS_Economy* Economy = GetWorld() ? GetWorld()->GetSubsystem<UWS_Economy>() : nullptr;
	if (!Economy)
	{
		return INDEX_NONE;
	}

	const FBuildingDefinitionRow* Def = FindBuildingDef(BuildingId);
	if (!Def)
	{
		return INDEX_NONE;
	}

	FString Reason;
	if (!ValidatePlacement(BuildingId, Location, YawDeg, Reason))
	{
		return INDEX_NONE;
	}

	UGIS_DataRegistry* Registry = GetWorld()->GetGameInstance()->GetSubsystem<UGIS_DataRegistry>();
	const FWorkDefinitionRow* WorkDef = Registry ? Registry->GetWorkDef(Def->BuildWorkId) : nullptr;
	if (!WorkDef)
	{
		return INDEX_NONE;
	}

	if (!IEidosEconomyAccess::Execute_CanAfford(Economy, WorkDef->Costs))
	{
		return INDEX_NONE;
	}

	FWorkRequest Request;
	Request.WorkId = Def->BuildWorkId;
	Request.Mode = EWorkRequestMode::Count;
	Request.RemainingCount = 1;
	Request.Priority = 100;

	const int32 RequestId = WorkSubsystem->AddWorkRequest(Request);
	if (RequestId == INDEX_NONE)
	{
		return INDEX_NONE;
	}

	CreateConstructionSite(BuildingId, Location, YawDeg, RequestId);
	return RequestId;
}

void UWS_Building::HandleWorkRequestStateChanged(int32 WorkRequestId, EWorkRequestLifecycleState NewState)
{
	FConstructionSiteState* Site = FindConstructionSiteByRequestId(WorkRequestId);
	if (!Site)
	{
		return;
	}

	switch (NewState)
	{
	case EWorkRequestLifecycleState::Queued:
		Site->State = EConstructionSiteLifecycle::Queued;
		break;
	case EWorkRequestLifecycleState::Active:
		Site->State = EConstructionSiteLifecycle::InProgress;
		break;
	case EWorkRequestLifecycleState::Completed:
		Site->State = EConstructionSiteLifecycle::AwaitingFinalization;
		PendingFinalizeSiteIds.AddUnique(Site->SiteId);
		break;
	case EWorkRequestLifecycleState::Cancelled:
		Site->State = EConstructionSiteLifecycle::Cancelled;
		if (Site->SiteActor.IsValid())
		{
			Site->SiteActor->Destroy();
			Site->SiteActor = nullptr;
		}
		break;
	case EWorkRequestLifecycleState::Failed:
		Site->State = EConstructionSiteLifecycle::Failed;
		break;
	default:
		break;
	}
}

void UWS_Building::FinalizeBuilding(int32 SiteId)
{
	FConstructionSiteState* Site = FindConstructionSiteById(SiteId);
	if (!Site || Site->State == EConstructionSiteLifecycle::Completed)
	{
		return;
	}

	if (Site->SiteActor.IsValid())
	{
		Site->SiteActor->Destroy();
		Site->SiteActor = nullptr;
	}

	SpawnFinalBuildingActor(*Site, true);
	Site->State = EConstructionSiteLifecycle::Completed;
}

void UWS_Building::CleanupInvalidActors()
{
	for (FConstructionSiteState& Site : ConstructionSites)
	{
		if (!Site.SiteActor.IsValid())
		{
			Site.SiteActor = nullptr;
		}
		if (!Site.FinalActor.IsValid())
		{
			Site.FinalActor = nullptr;
		}
	}
}

void UWS_Building::DestroyAllRuntimeActors()
{
	for (FConstructionSiteState& Site : ConstructionSites)
	{
		if (Site.SiteActor.IsValid())
		{
			Site.SiteActor->Destroy();
			Site.SiteActor = nullptr;
		}
		if (Site.FinalActor.IsValid())
		{
			Site.FinalActor->Destroy();
			Site.FinalActor = nullptr;
		}
	}
}

void UWS_Building::SpawnConstructionSiteActor(FConstructionSiteState& Site)
{
	if (Site.SiteActor.IsValid())
	{
		return;
	}

	const FBuildingDefinitionRow* Def = FindBuildingDef(Site.BuildingId);
	if (!Def || Def->ConstructionSiteActorClass.IsNull())
	{
		return;
	}

	UClass* SpawnClass = Def->ConstructionSiteActorClass.LoadSynchronous();
	if (!SpawnClass)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* Spawned = GetWorld()->SpawnActor<AActor>(
		SpawnClass,
		Site.Location + FVector(0.f, 0.f, Def->ZOffset),
		FRotator(0.f, Site.YawDeg, 0.f),
		Params);

	Site.SiteActor = Spawned;
	if (AConstructionSiteActor* SiteActor = Cast<AConstructionSiteActor>(Spawned))
	{
		SiteActor->InitializeConstructionSite(Site.SiteId, Site.BuildingId, Site.WorkRequestId);
		SiteActor->SetFootprint(Def->Footprint);
		SiteActor->SetPreviewValid(true);
	}
}

void UWS_Building::SpawnFinalBuildingActor(FConstructionSiteState& Site, bool bRegisterAutoWorks)
{
	if (Site.FinalActor.IsValid())
	{
		return;
	}

	const FBuildingDefinitionRow* Def = FindBuildingDef(Site.BuildingId);
	if (!Def || Def->BuildingActorClass.IsNull())
	{
		return;
	}

	UClass* SpawnClass = Def->BuildingActorClass.LoadSynchronous();
	if (!SpawnClass)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* Spawned = GetWorld()->SpawnActor<AActor>(
		SpawnClass,
		Site.Location + FVector(0.f, 0.f, Def->ZOffset),
		FRotator(0.f, Site.YawDeg, 0.f),
		Params);

	Site.FinalActor = Spawned;
	if (ABuildingActorBase* BuildingActor = Cast<ABuildingActorBase>(Spawned))
	{
		BuildingActor->InitializeBuilding(Site.BuildingId, 1);
		BuildingActor->SetFootprint(Def->Footprint);
		BuildingActor->SetBuildingActive(true);
	}

	if (bRegisterAutoWorks && Spawned)
	{
		RegisterAutoWorksForBuilding(Def->BuildingId, Spawned->GetActorLocation());
	}
}

void UWS_Building::RespawnActorsForSite(FConstructionSiteState& Site, bool bRegisterAutoWorks)
{
	switch (Site.State)
	{
	case EConstructionSiteLifecycle::Queued:
	case EConstructionSiteLifecycle::InProgress:
	case EConstructionSiteLifecycle::AwaitingFinalization:
		SpawnConstructionSiteActor(Site);
		break;
	case EConstructionSiteLifecycle::Completed:
		SpawnFinalBuildingActor(Site, bRegisterAutoWorks);
		break;
	case EConstructionSiteLifecycle::Cancelled:
	case EConstructionSiteLifecycle::Failed:
	default:
		break;
	}
}

void UWS_Building::SimPlan_Implementation(USimCommandBuffer* CommandBuffer, float FixedDeltaSeconds)
{
	PlannedFinalizeSiteIds = PendingFinalizeSiteIds;
	PendingFinalizeSiteIds.Reset();
}

void UWS_Building::SimCommit_Implementation(USimCommandBuffer* CommandBuffer, float FixedDeltaSeconds)
{
	if (!CommandBuffer)
	{
		return;
	}

	TWeakObjectPtr<UWS_Building> WeakThis(this);
	for (int32 SiteId : PlannedFinalizeSiteIds)
	{
		CommandBuffer->Enqueue([WeakThis, SiteId]()
		{
			if (UWS_Building* Building = WeakThis.Get())
			{
				Building->FinalizeBuilding(SiteId);
			}
		});
	}
}

void UWS_Building::SimPost_Implementation(float FixedDeltaSeconds)
{
	CleanupInvalidActors();
}

void UWS_Building::WriteToSnapshot_Implementation(FEidosWorldSnapshot& OutSnapshot) const
{
	OutSnapshot.Buildings.NextSiteId = NextSiteId;
	OutSnapshot.Buildings.Sites.Reset();

	for (const FConstructionSiteState& Site : ConstructionSites)
	{
		FEidosConstructionSiteSnapshot SiteSnapshot;
		SiteSnapshot.SiteId = Site.SiteId;
		SiteSnapshot.BuildingId = Site.BuildingId;
		SiteSnapshot.Location = Site.Location;
		SiteSnapshot.YawDeg = Site.YawDeg;
		SiteSnapshot.WorkRequestId = Site.WorkRequestId;
		SiteSnapshot.State = Site.State;
		OutSnapshot.Buildings.Sites.Add(SiteSnapshot);
	}
}

void UWS_Building::ApplySnapshot_Implementation(const FEidosWorldSnapshot& Snapshot)
{
	DestroyAllRuntimeActors();
	ConstructionSites.Reset();
	PendingFinalizeSiteIds.Reset();
	PlannedFinalizeSiteIds.Reset();

	NextSiteId = FMath::Max(1, Snapshot.Buildings.NextSiteId);
	for (const FEidosConstructionSiteSnapshot& SiteSnapshot : Snapshot.Buildings.Sites)
	{
		FConstructionSiteState Site;
		Site.SiteId = SiteSnapshot.SiteId;
		Site.BuildingId = SiteSnapshot.BuildingId;
		Site.Location = SiteSnapshot.Location;
		Site.YawDeg = SiteSnapshot.YawDeg;
		Site.WorkRequestId = SiteSnapshot.WorkRequestId;
		Site.State = SiteSnapshot.State;
		ConstructionSites.Add(Site);
		NextSiteId = FMath::Max(NextSiteId, Site.SiteId + 1);
	}

	for (FConstructionSiteState& Site : ConstructionSites)
	{
		RespawnActorsForSite(Site, false);
		if (Site.State == EConstructionSiteLifecycle::AwaitingFinalization)
		{
			PendingFinalizeSiteIds.AddUnique(Site.SiteId);
		}
	}
}

void UWS_Building::RegisterAutoWorksForBuilding(FName BuildingId, const FVector& BuildingLocation)
{
	if (!WorkSubsystem)
	{
		return;
	}

	const FBuildingDefinitionRow* Def = FindBuildingDef(BuildingId);
	if (!Def)
	{
		return;
	}

	for (const FAutoWorkEntry& Auto : Def->AutoWorks)
	{
		if (Auto.WorkId.IsNone())
		{
			continue;
		}

		FWorkRequest Request;
		Request.WorkId = Auto.WorkId;
		Request.Priority = Auto.Priority;
		Request.Mode = EWorkRequestMode::Count;
		Request.RemainingCount = FMath::Max(1, Auto.InitialCount);
		WorkSubsystem->AddWorkRequest(Request);
	}
}
