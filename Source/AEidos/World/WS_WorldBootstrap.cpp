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
#include "World/Settlement/WS_PortalDirector.h"
#include "Combat/WS_CombatDirector.h"
#include "GameFramework/GameModeBase.h"
#include "Framework/EidosGameMode.h"
#include "Framework/MenuGameMode.h"
#include "Settlement/WS_Population.h"
#include "World/Settlement/WS_Work.h"
#include "World/Settlement/WS_Economy.h"
#include "World/Settlement/WS_Sustenance.h"
#include "World/Settlement/WS_Building.h"
#include "World/Settlement/WS_Research.h"
#include "World/Settlement/WS_Population.h"

// 로그 카테고리(있으면 교체)
DEFINE_LOG_CATEGORY_STATIC(LogWorldBootstrap, Log, All);



void UWS_WorldBootstrap::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	CachedWorld = GetWorld();
	bBootstrapScheduled = false;
	bBootstrapping = false;
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

	UE_LOG(LogWorldBootstrap, Log, TEXT("[WorldBootstrap] OnWorldBeginPlay. Map=%s"), *InWorld.GetMapName());
	
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
	if (bBootstrapped || bBootstrapping || bBootstrapScheduled)
		return;

	UWorld* World = CachedWorld.Get();
	if (!World)
		return;

	bBootstrapScheduled = true;
	
	World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UWS_WorldBootstrap::BeginBootstrap));
}

void UWS_WorldBootstrap::BeginBootstrap()
{
	if (bBootstrapped || bBootstrapping)
		return;

	UWorld* World = CachedWorld.Get();
	if (!World)
		return;
	
	if (!IsGameWorld(World))
	{
		UE_LOG(LogWorldBootstrap, Warning, TEXT("[WorldBootstrap] BeginBootstrap called but not GameWorld. Map=%s"), *World->GetMapName());
		return;
	}

	bBootstrapping = true;

	UE_LOG(LogWorldBootstrap, Log, TEXT("[WorldBootstrap] BeginBootstrap START. Map=%s"), *World->GetMapName());

	UGameInstance* GI = World->GetGameInstance();
	if (!GI)
	{
		UE_LOG(LogWorldBootstrap, Error, TEXT("[WorldBootstrap] GameInstance is null."));
		return;
	}
	
	if (UGIS_DataRegistry* DR1 = GI->GetSubsystem<UGIS_DataRegistry>())
	{
		DR1->EnsureReady([this](bool bOk)
		{
			ContinueBootstrapAfterDataRegistryReady(bOk);
		}); 
	}
}

void UWS_WorldBootstrap::ContinueBootstrapAfterDataRegistryReady(bool bOk)
{
	UWorld* World = CachedWorld.Get();
	if (!World)
	{
		bBootstrapping = false;
		return;
	}

	UGameInstance* GI = World->GetGameInstance();
	if (!GI)
	{
		bBootstrapping = false;
		return;
	}

	if (UWS_Building* BuildingWS = GetWorld()->GetSubsystem<UWS_Building>())
	{
		BuildingWS->LoadBuildingDefs();
	}

	if (UWS_Work* WorkWS = GetWorld()->GetSubsystem<UWS_Work>())
	{
		WorkWS->LoadWorkDefs();
	}
	if (UWS_Research* ResearchWS = GetWorld()->GetSubsystem<UWS_Research>())
	{
		ResearchWS->LoadResearchDefs();
	}

	if (!bOk)
	{
		UGIS_DataRegistry* DR = GI->GetSubsystem<UGIS_DataRegistry>();
		const FString Reason = DR ? DR->GetNotReadyReason() : TEXT("Unknown");
		UE_LOG(LogWorldBootstrap, Error, TEXT("[WorldBootstrap] DataRegistry not ready: %s"), *Reason);
		
		bBootstrapping = false;
		return;
	}

	UE_LOG(LogWorldBootstrap, Log, TEXT("[WorldBootstrap] DataRegistry READY. Continue..."));
	
	if (UGIS_SaveLoad* SL = GI->GetSubsystem<UGIS_SaveLoad>())
	{
		SL->ApplyPendingSnapshotToWorld(*World);
	}

	UWS_SimulationOrchestrator* Orch = World->GetSubsystem<UWS_SimulationOrchestrator>();
	if (!Orch)
	{
		UE_LOG(LogWorldBootstrap, Error, TEXT("[WorldBootstrap] WS_SimulationOrchestrator missing. Cannot start main loop."));
		bBootstrapping = false;
		return;
	}
	
	Orch->RegisterSimSystem(World->GetSubsystem<UWS_SettlementSpace>());
	Orch->RegisterSimSystem(World->GetSubsystem<UWS_Work>());
	Orch->RegisterSimSystem(World->GetSubsystem<UWS_Economy>());
	Orch->RegisterSimSystem(World->GetSubsystem<UWS_Sustenance>());
	Orch->RegisterSimSystem(World->GetSubsystem<UWS_Population>());
	Orch->RegisterSimSystem(World->GetSubsystem<UWS_Building>());
	Orch->RegisterSimSystem(World->GetSubsystem<UWS_PortalDirector>());
	Orch->RegisterSimSystem(World->GetSubsystem<UWS_CombatDirector>());
	World->GetSubsystem<UWS_Population>()->EnsureTestPageSpawned(); //테스트용 page 하나 소환 보장
	
	Orch->StartMainLoop();

	FinalizeBootstrap();
}

void UWS_WorldBootstrap::FinalizeBootstrap()
{
	bBootstrapping = false;
	bBootstrapped = true;

	UWorld* World = CachedWorld.Get();
	const FString MapName = World ? World->GetMapName() : TEXT("NULL");
	UE_LOG(LogWorldBootstrap, Log, TEXT("[WorldBootstrap] BeginBootstrap DONE. Map=%s"), *MapName);
}

