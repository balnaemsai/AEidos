// Fill out your copyright notice in the Description page of Project Settings.


#include "World/Settlement/WS_Building.h"

#include "Data/GIS_DataRegistry.h"
#include "Data/Definitions/BuildingDefinitionRow.h"
#include "World/Settlement/WS_Work.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Simulation/SimCommandBuffer.h"
#include "World/Settlement/Building/ConstructionSiteActor.h"
#include "World/Settlement/Building/BuildingActorBase.h"

void UWS_Building::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	WorkSubsystem = GetWorld()->GetSubsystem<UWS_Work>();
	LoadBuildingDefs();
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

	UDataTable* DT = Registry->FindDataTableByName(TEXT("DT_Building"));
	if (!DT)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Building] DT_Building not found"));
		return;
	}

	TArray<FBuildingDefinitionRow*> Rows;
	DT->GetAllRows(TEXT("WS_Building::LoadBuildingDefs"), Rows);

	for (const FBuildingDefinitionRow* Row : Rows)
	{
		if (!Row)
		{
			continue;
		}
		BuildingDefs.Add(Row->BuildingId, *Row);
	}

	UE_LOG(LogTemp, Log, TEXT("[Building] Loaded defs: %d"), BuildingDefs.Num());
}

const FBuildingDefinitionRow* UWS_Building::FindBuildingDef(FName BuildingId) const
{
	if (const FBuildingDefinitionRow* Found = BuildingDefs.Find(BuildingId))
	{
		return Found;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Building] Missing BuildingDef: %s"), *BuildingId.ToString());
	return nullptr;
}

bool UWS_Building::IntersectsAnyPlacedOrConstruction(const FBuildingDefinitionRow& Def, FVector Location) const
{
	const FVector2D Half = Def.Footprint * 0.5f;

	auto Overlap2D = [](const FVector2D& ACenter, const FVector2D& AHalf, const FVector2D& BCenter,
	                    const FVector2D& BHalf)
	{
		return FMath::Abs(ACenter.X - BCenter.X) <= (AHalf.X + BHalf.X)
			&& FMath::Abs(ACenter.Y - BCenter.Y) <= (AHalf.Y + BHalf.Y);
	};

	const FVector2D Center(Location.X, Location.Y);

	// 공사장끼리 겹침 방지
	for (const FConstructionSiteState& Site : ConstructionSites)
	{
		if (Site.bCompleted)
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

	// 완공된 건물 Actor와 겹침 방지
	for (const FConstructionSiteState& Site : ConstructionSites)
	{
		if (!Site.bCompleted)
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

	// TODO:
	// 1) SettlementSpace 범위 체크
	// 2) 경로 봉쇄 체크
	// 3) 지형/바닥 조건 체크

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
	Site.bCompleted = false;

	const FBuildingDefinitionRow* Def = FindBuildingDef(BuildingId);
	if (Def && Def->ConstructionSiteActorClass.IsValid())
	{
		UClass* SpawnClass = Def->ConstructionSiteActorClass.LoadSynchronous();
		if (SpawnClass)
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			AActor* Spawned = GetWorld()->SpawnActor<AActor>(
				SpawnClass,
				Location + FVector(0.f, 0.f, Def->ZOffset),
				FRotator(0.f, YawDeg, 0.f),
				Params);

			Site.SiteActor = Spawned;

			if (AConstructionSiteActor* SiteActor = Cast<AConstructionSiteActor>(Spawned))
			{
				SiteActor->InitializeConstructionSite(Site.SiteId, BuildingId, WorkRequestId);
				SiteActor->SetFootprint(Def->Footprint);
				SiteActor->SetPreviewValid(true);
			}
		}
	}

	ConstructionSites.Add(Site);
	return Site.SiteId;
}

int32 UWS_Building::RequestBuild(FName BuildingId, FVector Location, float YawDeg)
{
	if (!WorkSubsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Building] WorkSubsystem missing"));
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
		UE_LOG(LogTemp, Warning, TEXT("[Building] RequestBuild failed: %s"), *Reason);
		return INDEX_NONE;
	}

	FWorkRequest Req;
	Req.WorkId = Def->BuildWorkId;
	Req.Mode = EWorkRequestMode::Count;
	Req.RemainingCount = 1;
	Req.Priority = 100;

	const int32 RequestId = WorkSubsystem->AddWorkRequest(Req);
	if (RequestId == INDEX_NONE)
	{
		return INDEX_NONE;
	}

	CreateConstructionSite(BuildingId, Location, YawDeg, RequestId);

	UE_LOG(LogTemp, Log, TEXT("[Building] RequestBuild BuildingId=%s RequestId=%d"),
	       *BuildingId.ToString(), RequestId);

	return RequestId;
}

void UWS_Building::FinalizeBuilding(int32 SiteId)
{
	FConstructionSiteState* Site = ConstructionSites.FindByPredicate([&](const FConstructionSiteState& S)
	{
		return S.SiteId == SiteId;
	});

	if (!Site || Site->bCompleted)
	{
		return;
	}

	const FBuildingDefinitionRow* Def = FindBuildingDef(Site->BuildingId);
	if (!Def)
	{
		return;
	}

	if (Site->SiteActor.IsValid())
	{
		Site->SiteActor->Destroy();
		Site->SiteActor = nullptr;
	}

	if (Def->BuildingActorClass.IsValid())
	{
		UClass* SpawnClass = Def->BuildingActorClass.LoadSynchronous();
		if (SpawnClass)
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			AActor* Spawned = GetWorld()->SpawnActor<AActor>(
				SpawnClass,
				Site->Location + FVector(0.f, 0.f, Def->ZOffset),
				FRotator(0.f, Site->YawDeg, 0.f),
				Params);

			Site->FinalActor = Spawned;
			
			if (ABuildingActorBase* BuildingActor = Cast<ABuildingActorBase>(Spawned))
			{
				BuildingActor->InitializeBuilding(Site->BuildingId, 1);
				BuildingActor->SetFootprint(Def->Footprint);
				BuildingActor->SetBuildingActive(true);
			}
		}
	}

	Site->bCompleted = true;

	UE_LOG(LogTemp, Log, TEXT("[Building] Finalized SiteId=%d BuildingId=%s"),
	       SiteId, *Site->BuildingId.ToString());
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

void UWS_Building::SimPlan_Implementation(USimCommandBuffer* CommandBuffer, float FixedDeltaSeconds)
{
	PlannedCompletedSiteIds.Reset();

	if (!WorkSubsystem)
	{
		return;
	}

	// 공사 Work 요청이 더 이상 queue에도 없고 active에도 없으면 완료로 본다.
	for (const FConstructionSiteState& Site : ConstructionSites)
	{
		if (Site.bCompleted)
		{
			continue;
		}

		const bool bQueued = WorkSubsystem->HasQueuedRequest(Site.WorkRequestId);
		const bool bActive = WorkSubsystem->HasActiveInstanceForRequest(Site.WorkRequestId);

		if (!bQueued && !bActive)
		{
			PlannedCompletedSiteIds.Add(Site.SiteId);
		}
	}
}

void UWS_Building::SimCommit_Implementation(USimCommandBuffer* CommandBuffer, float FixedDeltaSeconds)
{
	if (!CommandBuffer)
	{
		return;
	}

	TWeakObjectPtr<UWS_Building> WeakThis(this);

	for (const int32 SiteId : PlannedCompletedSiteIds)
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
	// MVP 단계에서는 건설 사이트/완공 건물 최소 상태만 별도 직렬화 권장
	// 지금은 생략. 이후 Building JSON snapshot으로 확장.
}

void UWS_Building::ApplySnapshot_Implementation(const FEidosWorldSnapshot& Snapshot)
{
	// MVP 단계에서는 생략.
	// 이후 ConstructionSites/Completed buildings를 복원하면서 Actor respawn 처리.
}
