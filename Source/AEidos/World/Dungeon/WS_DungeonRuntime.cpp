// Fill out your copyright notice in the Description page of Project Settings.


#include "World/Dungeon/WS_DungeonRuntime.h"

#include "Data/GIS_DataRegistry.h"
#include "Data/Definitions/BuildingDefinitionRow.h"
#include "Engine/Level.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Entities/Page/PageCharacter.h"
#include "Entities/Page/Components/StatsComponent.h"
#include "GameFramework/PlayerStart.h"
#include "Components/CapsuleComponent.h"
#include "World/Dungeon/DungeonSettlementPreset.h"
#include "World/Settlement/TerritoryChunkActor.h"
#include "World/Settlement/WS_SettlementSpace.h"

namespace
{
	FTransform ResolveGroundedCharacterTransform(UWorld* World, const FTransform& DesiredTransform, const ACharacter* CharacterTemplate)
	{
		if (!World || !CharacterTemplate)
		{
			return DesiredTransform;
		}

		const UCapsuleComponent* Capsule = CharacterTemplate->GetCapsuleComponent();
		const float CapsuleHalfHeight = Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 88.f;
		const FVector BaseLocation = DesiredTransform.GetLocation();
		const FVector TraceStart = BaseLocation + FVector(0.f, 0.f, 500.f);
		const FVector TraceEnd = BaseLocation - FVector(0.f, 0.f, 1500.f);

		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(DungeonGroundSnap), false, CharacterTemplate);
		if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params))
		{
			FTransform GroundedTransform = DesiredTransform;
			GroundedTransform.SetLocation(FVector(
				BaseLocation.X,
				BaseLocation.Y,
				Hit.ImpactPoint.Z + CapsuleHalfHeight + 2.f));
			return GroundedTransform;
		}

		return DesiredTransform;
	}
}

UWS_DungeonRuntime::UWS_DungeonRuntime()
{
	DefaultDungeonLevel = TSoftObjectPtr<UWorld>(
		FSoftObjectPath(TEXT("/Script/Engine.World'/Game/Maps/DungeonRuntimeBase.DungeonRuntimeBase'")));
	DefaultSettlementPreset = TSoftObjectPtr<UDungeonSettlementPreset>(
		FSoftObjectPath(TEXT("/Game/Data/DA_TestDungeon.DA_TestDungeon")));
	DefaultEnemyPageClass = TSoftClassPtr<APageCharacter>(
		FSoftObjectPath(TEXT("/Game/Blueprints/BP_PageCharacter.BP_PageCharacter_C")));
}

void UWS_DungeonRuntime::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ResetActiveSession();
}

bool UWS_DungeonRuntime::EnterDungeonForPortal(int32 PortalId, APageCharacter* EnteringPage)
{
	if (!GetWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("[DungeonRuntime] EnterDungeonForPortal failed: world missing"));
		return false;
	}

	if (!EnteringPage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DungeonRuntime] EnterDungeonForPortal failed: page missing"));
		return false;
	}

	if (EnteringPage->IsInDungeon())
	{
		UE_LOG(LogTemp, Warning, TEXT("[DungeonRuntime] EnterDungeonForPortal failed: page already in dungeon"));
		return false;
	}

	if (HasActiveDungeon())
	{
		UE_LOG(LogTemp, Warning, TEXT("[DungeonRuntime] EnterDungeonForPortal failed: active dungeon already exists"));
		return false;
	}

	if (DefaultDungeonLevel.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("[DungeonRuntime] EnterDungeonForPortal failed: DefaultDungeonLevel missing"));
		return false;
	}

	TSoftObjectPtr<UWorld> DungeonLevelToLoad = DefaultDungeonLevel;
	if (!DungeonLevelToLoad.IsNull() && !DungeonLevelToLoad.IsValid())
	{
		DungeonLevelToLoad.LoadSynchronous();
	}

	if (!DungeonLevelToLoad.IsValid())
	{
		const TSoftObjectPtr<UWorld> LegacyFallbackLevel(
			FSoftObjectPath(TEXT("/Script/Engine.World'/Game/Maps/TestMap.TestMap'")));
		UE_LOG(LogTemp, Warning,
			TEXT("[DungeonRuntime] Default dungeon level '%s' not found. Falling back to '%s'."),
			*DefaultDungeonLevel.ToString(),
			*LegacyFallbackLevel.ToString());
		DungeonLevelToLoad = LegacyFallbackLevel;
	}

	bool bLoadSucceeded = false;
	const FString LevelInstanceName = FString::Printf(TEXT("PortalDungeon_%d"), PortalId);
	ULevelStreamingDynamic* StreamingLevel = ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(
		GetWorld(),
		DungeonLevelToLoad,
		DungeonLevelTransform,
		bLoadSucceeded,
		LevelInstanceName);

	if (!bLoadSucceeded || !StreamingLevel)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DungeonRuntime] EnterDungeonForPortal failed: level load request failed"));
		return false;
	}

	ResetActiveSession();
	ActiveSession.PortalId = PortalId;
	ActiveSession.PageEntityId = EnteringPage->GetPageEntityId();
	ActiveSession.ReturnTransform = EnteringPage->GetActorTransform();
	ActiveSession.OccupyingPage = EnteringPage;
	ActiveSession.StreamingLevel = StreamingLevel;
	ActiveSession.bPageTransferred = false;

	EnteringPage->CurrentJobState = FPageJobState{};

	StreamingLevel->OnLevelShown.AddUniqueDynamic(this, &UWS_DungeonRuntime::HandleActiveDungeonLevelShown);

	UE_LOG(LogTemp, Log,
		TEXT("[DungeonRuntime] Requested dungeon load for PortalId=%d PageId=%d Level=%s"),
		PortalId,
		EnteringPage->GetPageEntityId(),
		*DungeonLevelToLoad.ToString());

	return true;
}

bool UWS_DungeonRuntime::HasActiveDungeon() const
{
	return ActiveSession.StreamingLevel.IsValid();
}

bool UWS_DungeonRuntime::IsPageInActiveDungeon(const APageCharacter* Page) const
{
	return Page && ActiveSession.OccupyingPage.Get() == Page && Page->IsInDungeon();
}

void UWS_DungeonRuntime::HandleActiveDungeonLevelShown()
{
	ULevelStreamingDynamic* StreamingLevel = ActiveSession.StreamingLevel.Get();
	APageCharacter* Page = ActiveSession.OccupyingPage.Get();
	if (!StreamingLevel || !Page)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DungeonRuntime] HandleActiveDungeonLevelShown failed: session invalid"));
		return;
	}

	ULevel* LoadedLevel = StreamingLevel->GetLoadedLevel();
	if (!LoadedLevel)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DungeonRuntime] HandleActiveDungeonLevelShown failed: loaded level missing"));
		return;
	}

	const FTransform EntryTransform = ResolveDungeonEntryTransform(LoadedLevel);
	SpawnPresetLayoutIntoDungeon(LoadedLevel);
	MovePageIntoDungeon(Page, EntryTransform);
	ActiveSession.bPageTransferred = true;

	UE_LOG(LogTemp, Log,
		TEXT("[DungeonRuntime] PageId=%d moved into dungeon for PortalId=%d at %s"),
		Page->GetPageEntityId(),
		ActiveSession.PortalId,
		*EntryTransform.GetLocation().ToString());
}

FTransform UWS_DungeonRuntime::ResolveDungeonEntryTransform(ULevel* LoadedLevel) const
{
	if (!DefaultSettlementPreset.IsNull())
	{
		if (const UDungeonSettlementPreset* Preset = DefaultSettlementPreset.LoadSynchronous())
		{
			if (!Preset->EntryTransform.GetLocation().IsNearlyZero() || !Preset->EntryTransform.GetRotation().Equals(FQuat::Identity))
			{
				return MakeDungeonWorldTransform(Preset->EntryTransform);
			}
		}
	}

	if (LoadedLevel)
	{
		for (AActor* Actor : LoadedLevel->Actors)
		{
			if (IsValid(Actor) && Actor->ActorHasTag(DungeonEntryTag))
			{
				return Actor->GetActorTransform();
			}
		}

		for (AActor* Actor : LoadedLevel->Actors)
		{
			if (APlayerStart* PlayerStart = Cast<APlayerStart>(Actor))
			{
				return PlayerStart->GetActorTransform();
			}
		}
	}

	return DungeonLevelTransform;
}

FTransform UWS_DungeonRuntime::MakeDungeonWorldTransform(const FTransform& LocalTransform) const
{
	const FVector WorldLocation = DungeonLevelTransform.TransformPosition(LocalTransform.GetLocation());
	const FQuat WorldRotation = DungeonLevelTransform.GetRotation() * LocalTransform.GetRotation();
	return FTransform(WorldRotation, WorldLocation, LocalTransform.GetScale3D());
}

void UWS_DungeonRuntime::SpawnPresetLayoutIntoDungeon(ULevel* LoadedLevel)
{
	if (!LoadedLevel)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DungeonRuntime] SpawnPresetLayoutIntoDungeon failed: loaded level missing"));
		return;
	}

	if (DefaultSettlementPreset.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("[DungeonRuntime] SpawnPresetLayoutIntoDungeon failed: DefaultSettlementPreset is null"));
		return;
	}

	UDungeonSettlementPreset* Preset = DefaultSettlementPreset.LoadSynchronous();
	if (!Preset)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[DungeonRuntime] SpawnPresetLayoutIntoDungeon failed: preset missing at %s"),
			*DefaultSettlementPreset.ToString());
		return;
	}

	SpawnDungeonChunks(LoadedLevel, Preset);
	SpawnDungeonBuildings(LoadedLevel, Preset);
	SpawnDungeonEnemies(LoadedLevel, Preset);

	UE_LOG(LogTemp, Log,
		TEXT("[DungeonRuntime] Spawned preset '%s' Chunks=%d Buildings=%d EnemySpawns=%d"),
		*GetNameSafe(Preset),
		Preset->OwnedChunks.Num(),
		Preset->Buildings.Num(),
		Preset->EnemySpawns.Num());
}

void UWS_DungeonRuntime::SpawnDungeonChunks(ULevel* LoadedLevel, const UDungeonSettlementPreset* Preset)
{
	if (!LoadedLevel || !Preset)
	{
		return;
	}

	UWS_SettlementSpace* SettlementSpace = GetWorld() ? GetWorld()->GetSubsystem<UWS_SettlementSpace>() : nullptr;
	if (!SettlementSpace)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DungeonRuntime] SpawnDungeonChunks failed: SettlementSpace missing"));
		return;
	}

	TSubclassOf<ATerritoryChunkActor> ChunkActorClass = SettlementSpace->GetChunkActorClass();
	if (!ChunkActorClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DungeonRuntime] SpawnDungeonChunks failed: ChunkActorClass missing"));
		return;
	}

	const float ChunkSizeCm = Preset->ChunkSizeCm > 0.f ? Preset->ChunkSizeCm : SettlementSpace->GetChunkSizeCm();

	for (const FIntPoint& Coord : Preset->OwnedChunks)
	{
		const FVector LocalLocation(Coord.X * ChunkSizeCm, Coord.Y * ChunkSizeCm, 0.f);
		const FTransform SpawnTransform = MakeDungeonWorldTransform(FTransform(FRotator::ZeroRotator, LocalLocation));

		FActorSpawnParameters Params;
		Params.OverrideLevel = LoadedLevel;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		if (ATerritoryChunkActor* ChunkActor = GetWorld()->SpawnActor<ATerritoryChunkActor>(ChunkActorClass, SpawnTransform, Params))
		{
			ChunkActor->InitChunk(Coord, ChunkSizeCm);
			ActiveSession.SpawnedActors.Add(ChunkActor);
		}
	}
}

void UWS_DungeonRuntime::SpawnDungeonBuildings(ULevel* LoadedLevel, const UDungeonSettlementPreset* Preset)
{
	if (!LoadedLevel || !Preset)
	{
		return;
	}

	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UGIS_DataRegistry* Registry = GameInstance ? GameInstance->GetSubsystem<UGIS_DataRegistry>() : nullptr;
	if (!Registry || !Registry->EnsureReadySync())
	{
		UE_LOG(LogTemp, Warning, TEXT("[DungeonRuntime] SpawnDungeonBuildings failed: DataRegistry not ready"));
		return;
	}

	for (const FDungeonSettlementBuildingPreset& Building : Preset->Buildings)
	{
		const FBuildingDefinitionRow* Def = Registry->GetBuildingDef(Building.BuildingId);
		if (!Def)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[DungeonRuntime] Missing building definition for preset BuildingId=%s"),
				*Building.BuildingId.ToString());
			continue;
		}

		UClass* BuildingClass = Def->BuildingActorClass.LoadSynchronous();
		if (!BuildingClass)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[DungeonRuntime] Missing building actor class for preset BuildingId=%s"),
				*Building.BuildingId.ToString());
			continue;
		}

		FActorSpawnParameters Params;
		Params.OverrideLevel = LoadedLevel;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		const FTransform SpawnTransform = MakeDungeonWorldTransform(Building.LocalTransform);
		if (AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(BuildingClass, SpawnTransform, Params))
		{
			ActiveSession.SpawnedActors.Add(SpawnedActor);
		}
	}
}

void UWS_DungeonRuntime::SpawnDungeonEnemies(ULevel* LoadedLevel, const UDungeonSettlementPreset* Preset)
{
	if (!LoadedLevel || !Preset)
	{
		return;
	}

	UClass* EnemyPageClass = DefaultEnemyPageClass.LoadSynchronous();
	if (!EnemyPageClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DungeonRuntime] SpawnDungeonEnemies failed: DefaultEnemyPageClass missing"));
		return;
	}

	for (const FDungeonEnemySpawnPreset& EnemySpawn : Preset->EnemySpawns)
	{
		const int32 SpawnCount = FMath::Max(1, EnemySpawn.SpawnCount);
		const FTransform BaseTransform = MakeDungeonWorldTransform(EnemySpawn.LocalTransform);

		for (int32 Index = 0; Index < SpawnCount; ++Index)
		{
			const float Angle = SpawnCount > 1 ? (2.f * PI * Index) / SpawnCount : 0.f;
			const FVector Offset(FMath::Cos(Angle) * 90.f, FMath::Sin(Angle) * 90.f, 0.f);
			FTransform SpawnTransform = BaseTransform;
			SpawnTransform.AddToTranslation(Offset);

			FActorSpawnParameters Params;
			Params.OverrideLevel = LoadedLevel;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

			if (APageCharacter* EnemyPage = GetWorld()->SpawnActor<APageCharacter>(EnemyPageClass, SpawnTransform, Params))
			{
				const FTransform GroundedTransform = ResolveGroundedCharacterTransform(GetWorld(), EnemyPage->GetActorTransform(), EnemyPage);
				EnemyPage->SetActorLocationAndRotation(
					GroundedTransform.GetLocation(),
					GroundedTransform.GetRotation().Rotator(),
					false,
					nullptr,
					ETeleportType::TeleportPhysics);
				EnemyPage->SetFaction(EPageFaction::Hostile);
				EnemyPage->SetIsInDungeon(true);
				EnemyPage->CurrentJobState = FPageJobState{};

				if (UStatsComponent* Stats = EnemyPage->GetStats())
				{
					Stats->RestoreToFull();
				}

				ActiveSession.SpawnedActors.Add(EnemyPage);
			}
		}
	}
}

void UWS_DungeonRuntime::ResetActiveSession()
{
	for (AActor* SpawnedActor : ActiveSession.SpawnedActors)
	{
		if (IsValid(SpawnedActor))
		{
			SpawnedActor->Destroy();
		}
	}

	if (ULevelStreamingDynamic* StreamingLevel = ActiveSession.StreamingLevel.Get())
	{
		StreamingLevel->OnLevelShown.RemoveDynamic(this, &UWS_DungeonRuntime::HandleActiveDungeonLevelShown);
	}

	ActiveSession = FDungeonSessionRuntime{};
}

void UWS_DungeonRuntime::MovePageIntoDungeon(APageCharacter* Page, const FTransform& EntryTransform)
{
	if (!Page)
	{
		return;
	}

	const FTransform GroundedTransform = ResolveGroundedCharacterTransform(GetWorld(), EntryTransform, Page);
	Page->SetActorLocationAndRotation(
		GroundedTransform.GetLocation(),
		GroundedTransform.GetRotation().Rotator(),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	Page->SetIsInDungeon(true);
}
