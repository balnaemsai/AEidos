// Fill out your copyright notice in the Description page of Project Settings.



#include "World/WS_WorldBootstrap.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Data/GIS_DataRegistry.h"
#include "Save/GIS_SaveLoad.h"
#include "UI/GIS_UIRouter.h"
#include "Simulation/WS_SimulationOrchestrator.h"
#include "World/Settlement/WS_SettlementSpace.h"
/*
#include "World/Settlement/WS_Economy.h"
#include "World/Settlement/WS_Building.h"
#include "World/Settlement/WS_Population.h"
#include "World/Settlement/WS_Work.h"
#include "World/Settlement/WS_Research.h"
#include "World/Settlement/WS_PortalDirector.h"
#include "World/Settlement/WS_RaidDirector.h"
*/
#include "Combat/WS_CombatDirector.h"
#include "GameFramework/GameModeBase.h"
#include "Framework/EidosGameMode.h"
#include "Framework/MenuGameMode.h"

// 로그 카테고리(있으면 교체)
DEFINE_LOG_CATEGORY_STATIC(LogWorldBootstrap, Log, All);



void UWS_WorldBootstrap::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	CachedWorld = GetWorld();
	bBootstrapScheduled = false;
	bBootstrapped = false;
}

void UWS_WorldBootstrap::Deinitialize()
{
	CachedWorld = nullptr;
	Super::Deinitialize();
}

void UWS_WorldBootstrap::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	CachedWorld = &InWorld;
	
	if (!IsGameWorld(&InWorld))
	{
		UE_LOG(LogWorldBootstrap, Log, TEXT("[WorldBootstrap] Skip (not GameWorld). Map=%s"), *InWorld.GetMapName());
		return;
	}
	
	ScheduleBootstrapNextTick();
}

bool UWS_WorldBootstrap::IsGameWorld(const UWorld* World) const
{
	if (!World)
		return false;

	if (!World->IsGameWorld())
		return false;
	
	const FString MapName = World->GetMapName(); 
	if (MapName.Contains(TEXT("Menu"), ESearchCase::IgnoreCase))
		return false;
	
	const AGameModeBase* GM = World->GetAuthGameMode();
	if (GM)
	{
		if (GM->IsA(AMenuGameMode::StaticClass()))
			return false;

		if (GM->IsA(AEidosGameMode::StaticClass()))
			return true;
	}
	
	if (MapName.Contains(TEXT("GameMap"), ESearchCase::IgnoreCase))
		return true;
	
	return true;
}

void UWS_WorldBootstrap::ScheduleBootstrapNextTick()
{
	if (bBootstrapped || bBootstrapScheduled)
		return;

	UWorld* World = CachedWorld.Get();
	if (!World)
		return;

	bBootstrapScheduled = true;
	
	World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UWS_WorldBootstrap::BeginBootstrap));
}

void UWS_WorldBootstrap::BeginBootstrap()
{
	if (bBootstrapped)
		return;

	UWorld* World = CachedWorld.Get();
	if (!World)
		return;
	
	if (!IsGameWorld(World))
	{
		UE_LOG(LogWorldBootstrap, Warning, TEXT("[WorldBootstrap] BeginBootstrap called but not GameWorld. Map=%s"), *World->GetMapName());
		return;
	}

	bBootstrapped = true;

	UE_LOG(LogWorldBootstrap, Log, TEXT("[WorldBootstrap] BeginBootstrap START. Map=%s"), *World->GetMapName());

	UGameInstance* GI = World->GetGameInstance();
	if (!GI)
	{
		UE_LOG(LogWorldBootstrap, Error, TEXT("[WorldBootstrap] GameInstance is null."));
		return;
	}
	
	if (UGIS_DataRegistry* GIS_DataRegistry = GI->GetSubsystem<UGIS_DataRegistry>())
	{
		//GIS_DataRegistry->EnsureReady(); 
	}
	
	if (UGIS_SaveLoad* GIS_SaveLoad = GI->GetSubsystem<UGIS_SaveLoad>())
	{
		/*
		if (GIS_SaveLoad->HasPendingSnapshot())
		{
			UE_LOG(LogWorldBootstrap, Log, TEXT("[WorldBootstrap] Applying PendingSnapshot..."));
			GIS_SaveLoad->ApplyPendingSnapshotToWorld(*World);
		}
		else
		{
			UE_LOG(LogWorldBootstrap, Log, TEXT("[WorldBootstrap] No PendingSnapshot. Build NewGame snapshot."));
			GIS_SaveLoad->BuildNewGameSnapshotIfNeeded();
			GIS_SaveLoad->ApplyPendingSnapshotToWorld(*World);
		}
		*/
	}
	
	UWS_SimulationOrchestrator* WS_Orch = World->GetSubsystem<UWS_SimulationOrchestrator>();
	if (!WS_Orch)
	{
		UE_LOG(LogWorldBootstrap, Error, TEXT("[WorldBootstrap] WS_SimulationOrchestrator missing. Cannot start main loop."));
		return;
	}

	/*
	WS_Orch->StartMainLoop(); 
	WS_Orch->BroadcastWorldReady(); 

	if (UGIS_UIRouter* GIS_UIRouter = GI->GetSubsystem<UGIS_UIRouter>())
	{
		GIS_UIRouter->RequestShowInGameHUD();
		GIS_UIRouter->SetUIStateReady();
	}
	
	// APlayerController* PC = World->GetFirstPlayerController();
	// if (AEidosPlayerController* EPC = Cast<AEidosPlayerController>(PC))
	// {
	//     EPC->SetControlPageFromLoadedState();
	// }
	*/

	UE_LOG(LogWorldBootstrap, Log, TEXT("[WorldBootstrap] BeginBootstrap DONE."));
}
