#include "World/Dungeon/DungeonSettlementAuthoringActor.h"

#include "Data/GIS_DataRegistry.h"
#include "Data/Definitions/BuildingDefinitionRow.h"
#include "Engine/DataTable.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Entities/Page/PageCharacter.h"
#include "World/Dungeon/DungeonAuthoringMarker.h"
#include "World/Dungeon/DungeonSettlementPreset.h"
#include "World/Settlement/TerritoryChunkActor.h"
#include "Components/SceneComponent.h"

ADungeonSettlementAuthoringActor::ADungeonSettlementAuthoringActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
}

#if WITH_EDITOR
FTransform ADungeonSettlementAuthoringActor::MakeLocalTransform(const FTransform& WorldTransform) const
{
	return WorldTransform.GetRelativeTransform(GetActorTransform());
}

void ADungeonSettlementAuthoringActor::CapturePresetFromLevel()
{
	if (!TargetPreset)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DungeonAuthoring] CapturePresetFromLevel failed: TargetPreset missing"));
		return;
	}

	UWorld* World = GetWorld();
	ULevel* ScopeLevel = GetLevel();
	if (!World || !ScopeLevel)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DungeonAuthoring] CapturePresetFromLevel failed: world or level missing"));
		return;
	}

	TMap<UClass*, FName> BuildingClassToId;
	if (bCaptureBuildings)
	{
		if (UGIS_DataRegistry* Registry = GetGameInstance() ? GetGameInstance()->GetSubsystem<UGIS_DataRegistry>() : nullptr)
		{
			if (Registry->EnsureReadySync())
			{
				if (UDataTable* BuildingTable = Registry->GetBuildingTable())
				{
					for (const FName& RowName : BuildingTable->GetRowNames())
					{
						if (const FBuildingDefinitionRow* Def = Registry->GetBuildingDef(RowName))
						{
							if (UClass* BuildingClass = Def->BuildingActorClass.LoadSynchronous())
							{
								BuildingClassToId.Add(BuildingClass, RowName);
							}
						}
					}
				}
			}
		}
	}

	TSet<FIntPoint> ChunkSet;
	TArray<FDungeonSettlementBuildingPreset> BuildingEntries;
	TArray<FDungeonEnemySpawnPreset> EnemySpawns;
	FTransform EntryTransform = FTransform::Identity;
	FTransform CoreTransform = FTransform::Identity;
	int32 DetectedChunkActors = 0;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor) || Actor == this)
		{
			continue;
		}

		if (bRestrictCaptureToSameLevel && Actor->GetLevel() != ScopeLevel)
		{
			continue;
		}

		if (bCaptureTerritoryChunks)
		{
			if (ATerritoryChunkActor* ChunkActor = Cast<ATerritoryChunkActor>(Actor))
			{
				++DetectedChunkActors;
				const FVector LocalLocation = MakeLocalTransform(ChunkActor->GetActorTransform()).GetLocation();
				const FIntPoint Coord(
					FMath::RoundToInt(LocalLocation.X / ChunkSizeCm),
					FMath::RoundToInt(LocalLocation.Y / ChunkSizeCm));
				ChunkSet.Add(Coord);
				continue;
			}
		}

		if (bCaptureMarkers)
		{
			if (ADungeonAuthoringMarker* Marker = Cast<ADungeonAuthoringMarker>(Actor))
			{
				switch (Marker->MarkerType)
				{
				case EDungeonAuthoringMarkerType::Entry:
					EntryTransform = MakeLocalTransform(Marker->GetActorTransform());
					break;
				case EDungeonAuthoringMarkerType::Core:
					CoreTransform = MakeLocalTransform(Marker->GetActorTransform());
					break;
				case EDungeonAuthoringMarkerType::EnemySpawn:
				{
					FDungeonEnemySpawnPreset& Spawn = EnemySpawns.AddDefaulted_GetRef();
					Spawn.LocalTransform = MakeLocalTransform(Marker->GetActorTransform());
					Spawn.SpawnCount = FMath::Max(1, Marker->EnemySpawnCount);
					break;
				}
				default:
					break;
				}
				continue;
			}
		}

		if (bCaptureBuildings)
		{
			for (const TPair<UClass*, FName>& Pair : BuildingClassToId)
			{
				if (Actor->GetClass()->IsChildOf(Pair.Key))
				{
					FDungeonSettlementBuildingPreset& Building = BuildingEntries.AddDefaulted_GetRef();
					Building.BuildingId = Pair.Value;
					Building.LocalTransform = MakeLocalTransform(Actor->GetActorTransform());
					break;
				}
			}
		}
	}

	TargetPreset->Modify();
	TargetPreset->ChunkSizeCm = ChunkSizeCm;
	TargetPreset->OwnedChunks = ChunkSet.Array();
	TargetPreset->OwnedChunks.Sort([](const FIntPoint& A, const FIntPoint& B)
	{
		return (A.X == B.X) ? (A.Y < B.Y) : (A.X < B.X);
	});

	BuildingEntries.Sort([](const FDungeonSettlementBuildingPreset& A, const FDungeonSettlementBuildingPreset& B)
	{
		return A.BuildingId.LexicalLess(B.BuildingId);
	});

	TargetPreset->Buildings = MoveTemp(BuildingEntries);
	TargetPreset->EntryTransform = EntryTransform;
	TargetPreset->CoreTransform = CoreTransform;
	TargetPreset->EnemySpawns = MoveTemp(EnemySpawns);
	TargetPreset->MarkPackageDirty();

	UE_LOG(LogTemp, Log,
		TEXT("[DungeonAuthoring] Captured preset '%s' Chunks=%d Buildings=%d EnemySpawns=%d DetectedChunkActors=%d SameLevelOnly=%d"),
		*GetNameSafe(TargetPreset),
		TargetPreset->OwnedChunks.Num(),
		TargetPreset->Buildings.Num(),
		TargetPreset->EnemySpawns.Num(),
		DetectedChunkActors,
		bRestrictCaptureToSameLevel ? 1 : 0);

	if (bCaptureTerritoryChunks && DetectedChunkActors == 0)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[DungeonAuthoring] No TerritoryChunkActor instances were found during capture. Check whether BP_TerritoryChunkActor is placed in the current world and whether SameLevelOnly should be disabled."));
	}
}
#endif
