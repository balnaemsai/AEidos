// Fill out your copyright notice in the Description page of Project Settings.


#include "World/Dungeon/WS_DungeonRuntime.h"

#include "Data/GIS_DataRegistry.h"
#include "Data/Definitions/BuildingDefinitionRow.h"
#include "Data/Definitions/ItemDefinitionRow.h"
#include "Data/Definitions/DungeonAttributeDefinitionRow.h"
#include "Data/Definitions/PortalDefinitionRow.h"
#include "Engine/Level.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Entities/Page/PageCharacter.h"
#include "Entities/Page/Components/StatsComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerStart.h"
#include "Components/CapsuleComponent.h"
#include "World/Dungeon/DungeonSettlementPreset.h"
#include "World/Dungeon/DungeonCoreActor.h"
#include "World/Dungeon/DungeonReturnPortalActor.h"
#include "World/Interaction/WorldBlockActor.h"
#include "World/Interaction/WorldItemBlockActor.h"
#include "World/Settlement/TerritoryChunkActor.h"
#include "World/Settlement/WS_PortalDirector.h"
#include "World/Settlement/WS_ItemStorage.h"
#include "World/Settlement/WS_SettlementSpace.h"
#include "World/Settlement/SettlementCoreActor.h"

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

	FTransform ResolveSettlementReturnTransform(UWorld* World, const FTransform& FallbackTransform)
	{
		if (!World)
		{
			return FallbackTransform;
		}

		for (TActorIterator<ASettlementCoreActor> It(World); It; ++It)
		{
			if (const ASettlementCoreActor* Core = *It; IsValid(Core))
			{
				return Core->GetActorTransform();
			}
		}

		return FallbackTransform;
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
	DefaultDungeonCoreClass = ADungeonCoreActor::StaticClass();
	DungeonReturnPortalClass = ADungeonReturnPortalActor::StaticClass();
}

void UWS_DungeonRuntime::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ResetActiveSession();
}

void UWS_DungeonRuntime::Deinitialize()
{
	ResetActiveSession();
	Super::Deinitialize();
}

bool UWS_DungeonRuntime::EnterDungeonForPortal(int32 PortalId, APageCharacter* EnteringPage)
{
	TArray<APageCharacter*> EnteringPages;
	if (EnteringPage)
	{
		EnteringPages.Add(EnteringPage);
	}
	return EnterDungeonForPortal(PortalId, EnteringPages);
}

bool UWS_DungeonRuntime::EnterDungeonForPortal(int32 PortalId, const TArray<APageCharacter*>& EnteringPages)
{
	if (!GetWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("[DungeonRuntime] EnterDungeonForPortal failed: world missing"));
		return false;
	}

	TArray<APageCharacter*> UniquePages;
	for (APageCharacter* Page : EnteringPages)
	{
		if (Page && !UniquePages.Contains(Page))
		{
			UniquePages.Add(Page);
		}
	}
	if (UniquePages.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[DungeonRuntime] EnterDungeonForPortal failed: no Pages supplied"));
		return false;
	}

	for (APageCharacter* Page : UniquePages)
	{
		if (Page->IsInDungeon() || !CanJoinActiveExpedition(Page))
		{
			UE_LOG(LogTemp, Warning, TEXT("[DungeonRuntime] EnterDungeonForPortal failed: PageId=%d cannot join"), Page->GetPageEntityId());
			return false;
		}
	}
	APageCharacter* EnteringPage = UniquePages[0];

	if (HasActiveDungeon())
	{
		// A destroyed core starts the escape window, rather than closing the expedition.
		// Pages may still re-enter through the original settlement portal until collapse.
		if (ActiveSession.PortalId != PortalId ||
			(ActiveSession.bCoreDestroyed && !IsDungeonCollapseActive()))
		{
			UE_LOG(LogTemp, Warning, TEXT("[DungeonRuntime] EnterDungeonForPortal failed: another dungeon is active"));
			return false;
		}

		bool bAddedAny = false;
		for (APageCharacter* Page : UniquePages)
		{
			bAddedAny |= AddPageToActiveDungeon(Page);
		}
		return bAddedAny;
	}

	FPortalState PortalState;
	if (UWS_PortalDirector* PortalDirector = GetWorld()->GetSubsystem<UWS_PortalDirector>();
		!PortalDirector || !PortalDirector->TryGetPortalState(PortalId, PortalState))
	{
		UE_LOG(LogTemp, Warning, TEXT("[DungeonRuntime] EnterDungeonForPortal failed: portal state missing PortalId=%d"), PortalId);
		return false;
	}

	if (DefaultDungeonLevel.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("[DungeonRuntime] EnterDungeonForPortal failed: DefaultDungeonLevel missing"));
		return false;
	}

	UDungeonSettlementPreset* SettlementPreset = ResolveSettlementPresetForPortal(PortalState);
	if (!SettlementPreset)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DungeonRuntime] EnterDungeonForPortal failed: no compatible settlement preset for PortalId=%d"), PortalId);
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
	ActiveSession.SettlementValueAtSpawn = PortalState.SettlementValueAtSpawn;
	ActiveSession.DungeonDifficulty = FMath::Max(0.5f, PortalState.DungeonDifficulty);
	ActiveSession.DungeonAttributes = PortalState.DungeonAttributes;
	ActiveSession.SettlementPreset = SettlementPreset;
	ActiveSession.SettlementPresetId = SettlementPreset->PresetId;
	// All expedition members return at the settlement core, not the location of
	// whichever Page happened to enter the portal first.
	ActiveSession.ReturnTransform = ResolveSettlementReturnTransform(GetWorld(), EnteringPage->GetActorTransform());
	for (APageCharacter* Page : UniquePages)
	{
		ActiveSession.DungeonPages.Add(Page);
		Page->CurrentJobState = FPageJobState{};
	}
	ActiveSession.StreamingLevel = StreamingLevel;
	ActiveSession.bPageTransferred = false;

	StreamingLevel->OnLevelShown.AddUniqueDynamic(this, &UWS_DungeonRuntime::HandleActiveDungeonLevelShown);

	UE_LOG(LogTemp, Log,
		TEXT("[DungeonRuntime] Requested dungeon load for PortalId=%d Difficulty=%.2f Preset=%s Pages=%d Level=%s"),
		PortalId,
		ActiveSession.DungeonDifficulty,
		*ActiveSession.SettlementPresetId.ToString(),
		UniquePages.Num(),
		*DungeonLevelToLoad.ToString());

	return true;
}

bool UWS_DungeonRuntime::HasActiveDungeon() const
{
	return ActiveSession.StreamingLevel.IsValid();
}

bool UWS_DungeonRuntime::IsPageInActiveDungeon(const APageCharacter* Page) const
{
	if (!Page || !Page->IsInDungeon())
	{
		return false;
	}

	// Original expedition pages live in the persistent level after being moved,
	// while spawned dungeon units belong to the streamed level. A captured unit
	// that is later recruited must be accepted by the return portal as well.
	if (ActiveSession.DungeonPages.ContainsByPredicate(
		[Page](const TWeakObjectPtr<APageCharacter>& Candidate)
		{
			return Candidate.Get() == Page;
		}))
	{
		return true;
	}

	const ULevelStreamingDynamic* StreamingLevel = ActiveSession.StreamingLevel.Get();
	return StreamingLevel && StreamingLevel->GetLoadedLevel() && Page->GetLevel() == StreamingLevel->GetLoadedLevel();
}

bool UWS_DungeonRuntime::IsActiveDungeonForPortal(int32 PortalId) const
{
	return HasActiveDungeon() && ActiveSession.PortalId == PortalId;
}

bool UWS_DungeonRuntime::IsDungeonCollapseActive() const
{
	return HasActiveDungeon() && ActiveSession.bCoreDestroyed && ActiveSession.CollapseEndTimeSeconds > 0.0;
}

float UWS_DungeonRuntime::GetDungeonCollapseRemainingSeconds() const
{
	if (!IsDungeonCollapseActive() || !GetWorld())
	{
		return 0.f;
	}

	return FMath::Max(0.f, static_cast<float>(ActiveSession.CollapseEndTimeSeconds - GetWorld()->GetTimeSeconds()));
}

int32 UWS_DungeonRuntime::GetActiveDungeonPageCount() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<APageCharacter>& WeakPage : ActiveSession.DungeonPages)
	{
		if (const APageCharacter* Page = WeakPage.Get(); Page && Page->IsInDungeon())
		{
			++Count;
		}
	}
	return Count;
}

void UWS_DungeonRuntime::HandleActiveDungeonLevelShown()
{
	ULevelStreamingDynamic* StreamingLevel = ActiveSession.StreamingLevel.Get();
	if (!StreamingLevel || ActiveSession.DungeonPages.IsEmpty())
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

	ActiveSession.EntryTransform = ResolveDungeonEntryTransform(LoadedLevel);
	SpawnPresetLayoutIntoDungeon(LoadedLevel);
	for (const TWeakObjectPtr<APageCharacter>& WeakPage : ActiveSession.DungeonPages)
	{
		if (APageCharacter* Page = WeakPage.Get())
		{
			MovePageIntoDungeon(Page, ActiveSession.EntryTransform);
		}
	}
	ActiveSession.bPageTransferred = true;

	UE_LOG(LogTemp, Log,
		TEXT("[DungeonRuntime] %d Page(s) moved into dungeon for PortalId=%d at %s"),
		ActiveSession.DungeonPages.Num(),
		ActiveSession.PortalId,
		*ActiveSession.EntryTransform.GetLocation().ToString());
}

FTransform UWS_DungeonRuntime::ResolveDungeonEntryTransform(ULevel* LoadedLevel) const
{
	if (const UDungeonSettlementPreset* Preset = ActiveSession.SettlementPreset)
	{
		if (!Preset->EntryTransform.GetLocation().IsNearlyZero() || !Preset->EntryTransform.GetRotation().Equals(FQuat::Identity))
		{
			return MakeDungeonWorldTransform(Preset->EntryTransform);
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

UDungeonSettlementPreset* UWS_DungeonRuntime::ResolveSettlementPresetForPortal(const FPortalState& PortalState) const
{
	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UGIS_DataRegistry* Registry = GameInstance ? GameInstance->GetSubsystem<UGIS_DataRegistry>() : nullptr;
	TArray<const FPortalDefinitionRow*> Candidates;
	if (Registry)
	{
		Registry->GetEligiblePortalDungeonPresets(PortalState.DungeonDifficulty, Candidates);
	}

	int32 BestSpecificity = INDEX_NONE;
	TArray<TPair<const FPortalDefinitionRow*, UDungeonSettlementPreset*>> BestCandidates;
	for (const FPortalDefinitionRow* Candidate : Candidates)
	{
		UDungeonSettlementPreset* Preset = Candidate ? Candidate->PresetAsset.LoadSynchronous() : nullptr;
		if (!Preset || !IsPresetCompatibleWithAttributes(*Preset, PortalState.DungeonAttributes))
		{
			continue;
		}

		const int32 Specificity = Preset->SupportedAttributeIds.IsEmpty() ? 0 : Preset->SupportedAttributeIds.Num();
		if (Specificity > BestSpecificity)
		{
			BestSpecificity = Specificity;
			BestCandidates.Reset();
		}
		if (Specificity == BestSpecificity)
		{
			BestCandidates.Emplace(Candidate, Preset);
		}
	}

	if (!BestCandidates.IsEmpty())
	{
		float TotalWeight = 0.f;
		for (const TPair<const FPortalDefinitionRow*, UDungeonSettlementPreset*>& Candidate : BestCandidates)
		{
			TotalWeight += Candidate.Key->SelectionWeight;
		}

		FRandomStream Stream(PortalState.DungeonSeed ^ 0x2A9D4F);
		float Roll = Stream.FRandRange(0.f, FMath::Max(TotalWeight, KINDA_SMALL_NUMBER));
		for (const TPair<const FPortalDefinitionRow*, UDungeonSettlementPreset*>& Candidate : BestCandidates)
		{
			Roll -= Candidate.Key->SelectionWeight;
			if (Roll <= 0.f)
			{
				return Candidate.Value;
			}
		}
		return BestCandidates.Last().Value;
	}

	UDungeonSettlementPreset* Fallback = DefaultSettlementPreset.IsNull() ? nullptr : DefaultSettlementPreset.LoadSynchronous();
	if (Fallback)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DungeonRuntime] No unified portal preset matched PortalId=%d. Using fallback preset '%s'."),
			PortalState.PortalId, *GetNameSafe(Fallback));
	}
	return Fallback;
}

bool UWS_DungeonRuntime::IsPresetCompatibleWithAttributes(const UDungeonSettlementPreset& Preset, const TArray<FDungeonAttributeWeight>& Attributes) const
{
	if (Preset.SupportedAttributeIds.IsEmpty() || Attributes.IsEmpty())
	{
		return true;
	}

	for (const FDungeonAttributeWeight& Attribute : Attributes)
	{
		if (!Attribute.AttributeId.IsNone() && !Preset.SupportedAttributeIds.Contains(Attribute.AttributeId))
		{
			return false;
		}
	}
	return true;
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

	UDungeonSettlementPreset* Preset = ActiveSession.SettlementPreset;
	if (!Preset)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[DungeonRuntime] SpawnPresetLayoutIntoDungeon failed: active settlement preset missing"));
		return;
	}

	SpawnDungeonChunks(LoadedLevel, Preset);
	SpawnDungeonBuildings(LoadedLevel, Preset);
	SpawnDungeonCore(LoadedLevel, Preset);
	SpawnDungeonEnemies(LoadedLevel, Preset);
	SpawnDungeonWorldBlocks(LoadedLevel, Preset);
	SpawnDungeonWorldItems(LoadedLevel, Preset);

	UE_LOG(LogTemp, Log,
		TEXT("[DungeonRuntime] Spawned preset '%s' Chunks=%d Buildings=%d EnemySpawns=%d WorldBlocks=%d LegacyWorldItems=%d"),
		*GetNameSafe(Preset),
		Preset->OwnedChunks.Num(),
		Preset->Buildings.Num(),
		Preset->EnemySpawns.Num(),
		Preset->WorldBlocks.Num(),
		Preset->WorldItems.Num());
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
void UWS_DungeonRuntime::SpawnDungeonCore(ULevel* LoadedLevel, const UDungeonSettlementPreset* Preset)
{
	if (!LoadedLevel || !Preset || !DefaultDungeonCoreClass)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.OverrideLevel = LoadedLevel;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FTransform SpawnTransform = MakeDungeonWorldTransform(Preset->CoreTransform);
	if (ADungeonCoreActor* CoreActor = GetWorld()->SpawnActor<ADungeonCoreActor>(DefaultDungeonCoreClass, SpawnTransform, Params))
	{
		TArray<FItemStack> Rewards;
		if (UGIS_DataRegistry* Registry = GetWorld()->GetGameInstance()->GetSubsystem<UGIS_DataRegistry>(); Registry && Registry->EnsureReadySync())
		{
			FItemStack PortalShard;
			PortalShard.ItemId = TEXT("PortalShard");
			PortalShard.Quantity = 1;
			for (const FDungeonAttributeWeight& Attribute : ActiveSession.DungeonAttributes)
			{
				if (const FDungeonAttributeDefinitionRow* Definition = Registry->GetDungeonAttributeDef(Attribute.AttributeId))
				{
					PortalShard.ItemId = Definition->CoreShardItemId;
					PortalShard.DungeonAttributes.Add(Attribute);
				}
			}
			if (PortalShard.IsValid() && !PortalShard.DungeonAttributes.IsEmpty()) Rewards.Add(PortalShard);
		}
		if (!Rewards.IsEmpty()) CoreActor->ConfigureCoreShardRewards(Rewards);
		CoreActor->OnCoreDestroyed.AddDynamic(this, &UWS_DungeonRuntime::HandleDungeonCoreDestroyed);
		ActiveSession.DungeonCore = CoreActor;
		ActiveSession.SpawnedActors.Add(CoreActor);
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
		const float Difficulty = FMath::Max(0.5f, ActiveSession.DungeonDifficulty);
		const int32 SpawnCount = FMath::Clamp(FMath::CeilToInt(FMath::Max(1, EnemySpawn.SpawnCount) * Difficulty), 1, 20);
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
					Stats->ApplyDifficultyScale(Difficulty);
				}

				ActiveSession.SpawnedActors.Add(EnemyPage);
			}
		}
	}
}

void UWS_DungeonRuntime::ResetActiveSession()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(DungeonCollapseTimerHandle);
	}

	if (ULevelStreamingDynamic* StreamingLevel = ActiveSession.StreamingLevel.Get())
	{
		StreamingLevel->OnLevelShown.RemoveDynamic(this, &UWS_DungeonRuntime::HandleActiveDungeonLevelShown);
		StreamingLevel->SetShouldBeVisible(false);
		StreamingLevel->SetShouldBeLoaded(false);
		StreamingLevel->SetIsRequestingUnloadAndRemoval(true);
	}

	for (AActor* SpawnedActor : ActiveSession.SpawnedActors)
	{
		if (IsValid(SpawnedActor))
		{
			SpawnedActor->Destroy();
		}
	}

	ActiveSession = FDungeonSessionRuntime{};
}

void UWS_DungeonRuntime::HandleDungeonCoreDestroyed(ADungeonCoreActor* DestroyedCore)
{
	if (!DestroyedCore || ActiveSession.DungeonCore.Get() != DestroyedCore)
	{
		return;
	}

	ULevel* LoadedLevel = ActiveSession.StreamingLevel.IsValid() ? ActiveSession.StreamingLevel->GetLoadedLevel() : nullptr;
	SpawnCoreShardWorldItems(DestroyedCore, LoadedLevel);
	OnDungeonCoreDestroyedForScenario.Broadcast(ActiveSession.PortalId);
	StartDungeonCollapse(DestroyedCore->GetActorTransform(), LoadedLevel);
}

void UWS_DungeonRuntime::SpawnCoreShardWorldItems(ADungeonCoreActor* DestroyedCore, ULevel* LoadedLevel)
{
	if (!DestroyedCore || !LoadedLevel || !GetWorld())
	{
		return;
	}

	UGameInstance* GameInstance = GetWorld()->GetGameInstance();
	UGIS_DataRegistry* Registry = GameInstance ? GameInstance->GetSubsystem<UGIS_DataRegistry>() : nullptr;
	TArray<FItemStack> Rewards = DestroyedCore->GetCoreShardRewards();
	if (Rewards.IsEmpty()) Rewards.Add({ DestroyedCore->GetCoreShardItemId(), DestroyedCore->GetCoreShardQuantity(), 0.f });
	for (int32 Index = 0; Index < Rewards.Num(); ++Index)
	{
		const FItemStack& Reward = Rewards[Index];
		const FItemDefinitionRow* ItemDef = Registry && Registry->EnsureReadySync() ? Registry->GetItemDef(Reward.ItemId) : nullptr;
		UClass* WorldItemClass = ItemDef ? ItemDef->WorldPickupClass.LoadSynchronous() : nullptr;
		if (!WorldItemClass || !WorldItemClass->IsChildOf(AWorldItemBlockActor::StaticClass())) continue;
		FTransform ShardTransform = DestroyedCore->GetActorTransform();
		const float Angle = Rewards.Num() > 1 ? 2.f * PI * Index / Rewards.Num() : 0.f;
		ShardTransform.AddToTranslation(ShardTransform.GetRotation().GetForwardVector() * 260.f + FVector(FMath::Cos(Angle) * 140.f, FMath::Sin(Angle) * 140.f, 30.f));
		FActorSpawnParameters Params;
		Params.OverrideLevel = LoadedLevel;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		if (AWorldItemBlockActor* Shard = GetWorld()->SpawnActor<AWorldItemBlockActor>(WorldItemClass, ShardTransform, Params))
		{
			Shard->InitializeWorldItemStack(Reward);
			ActiveSession.SpawnedActors.Add(Shard);
		}
	}
}

void UWS_DungeonRuntime::SpawnDungeonWorldItems(ULevel* LoadedLevel, const UDungeonSettlementPreset* Preset)
{
	if (!LoadedLevel || !Preset)
	{
		return;
	}

	for (const FDungeonWorldItemPreset& ItemPreset : Preset->WorldItems)
	{
		UClass* ItemActorClass = ItemPreset.ActorClass.LoadSynchronous();
		if (!ItemActorClass || !ItemActorClass->IsChildOf(AWorldItemBlockActor::StaticClass()))
		{
			UE_LOG(LogTemp, Warning, TEXT("[DungeonRuntime] Invalid world item class for ItemId=%s"),
				*ItemPreset.ItemId.ToString());
			continue;
		}

		FActorSpawnParameters Params;
		Params.OverrideLevel = LoadedLevel;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		if (AWorldItemBlockActor* WorldItem = GetWorld()->SpawnActor<AWorldItemBlockActor>(
			ItemActorClass, MakeDungeonWorldTransform(ItemPreset.LocalTransform), Params))
		{
			WorldItem->ApplyDungeonPresetData(
				ItemPreset.ItemId,
				ItemPreset.RemainingQuantity,
				ItemPreset.QuantityPerHarvest,
				ItemPreset.bCanPickUp,
				ItemPreset.bCanHarvest,
				ItemPreset.RequiredHarvestToolTag);
			ActiveSession.SpawnedActors.Add(WorldItem);
		}
	}
}

void UWS_DungeonRuntime::SpawnDungeonWorldBlocks(ULevel* LoadedLevel, const UDungeonSettlementPreset* Preset)
{
	if (!LoadedLevel || !Preset)
	{
		return;
	}

	for (const FDungeonWorldBlockPreset& BlockPreset : Preset->WorldBlocks)
	{
		UClass* BlockActorClass = BlockPreset.ActorClass.LoadSynchronous();
		if (!BlockActorClass || !BlockActorClass->IsChildOf(AWorldBlockActor::StaticClass()))
		{
			UE_LOG(LogTemp, Warning, TEXT("[DungeonRuntime] Invalid world block class for BlockId=%s"),
				*BlockPreset.BlockId.ToString());
			continue;
		}

		FActorSpawnParameters Params;
		Params.OverrideLevel = LoadedLevel;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		if (AWorldBlockActor* WorldBlock = GetWorld()->SpawnActor<AWorldBlockActor>(
			BlockActorClass, MakeDungeonWorldTransform(BlockPreset.LocalTransform), Params))
		{
			WorldBlock->ApplyDungeonBlockPresetData(
				BlockPreset.BlockId,
				BlockPreset.RemainingIntegrity,
				BlockPreset.Interactions);
			ActiveSession.SpawnedActors.Add(WorldBlock);
		}
	}
}

void UWS_DungeonRuntime::StartDungeonCollapse(const FTransform& CoreTransform, ULevel* LoadedLevel)
{
	if (ActiveSession.bCoreDestroyed || !GetWorld())
	{
		return;
	}

	ActiveSession.bCoreDestroyed = true;
	ActiveSession.CollapseEndTimeSeconds = GetWorld()->GetTimeSeconds() + DungeonCollapseDurationSeconds;

	if (DungeonReturnPortalClass && LoadedLevel)
	{
		FActorSpawnParameters Params;
		Params.OverrideLevel = LoadedLevel;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		FTransform PortalTransform = CoreTransform;
		PortalTransform.AddToTranslation(FVector(0.f, 0.f, 30.f));
		if (ADungeonReturnPortalActor* ReturnPortal = GetWorld()->SpawnActor<ADungeonReturnPortalActor>(DungeonReturnPortalClass, PortalTransform, Params))
		{
			ActiveSession.ReturnPortal = ReturnPortal;
			ActiveSession.SpawnedActors.Add(ReturnPortal);
		}
	}

	GetWorld()->GetTimerManager().SetTimer(
		DungeonCollapseTimerHandle,
		this,
		&UWS_DungeonRuntime::HandleDungeonCollapseExpired,
		DungeonCollapseDurationSeconds,
		false);

	UE_LOG(LogTemp, Log,
		TEXT("[DungeonRuntime] Core destroyed for PortalId=%d. Return portal opened; collapse in %.0f seconds."),
		ActiveSession.PortalId,
		DungeonCollapseDurationSeconds);
}

bool UWS_DungeonRuntime::ReturnPageFromActiveDungeon(APageCharacter* ReturningPage)
{
	if (!ReturningPage || !ReturningPage->IsFriendly() || !IsPageInActiveDungeon(ReturningPage) || !IsDungeonCollapseActive())
	{
		return false;
	}

	const FTransform ReturnTransform = ResolveGroundedCharacterTransform(GetWorld(), ActiveSession.ReturnTransform, ReturningPage);
	ReturningPage->SetActorLocationAndRotation(
		ReturnTransform.GetLocation(),
		ReturnTransform.GetRotation().Rotator(),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	ReturningPage->SetIsInDungeon(false);
	ReturningPage->CurrentJobState = FPageJobState{};
	if (UWS_ItemStorage* ItemStorage = GetWorld()->GetSubsystem<UWS_ItemStorage>())
	{
		ItemStorage->ConvertReturnResources(ReturningPage);
	}

	// Captives are not directly controllable, so operating the return portal
	// extracts every secured captive from this dungeon with the expedition.
	const int32 ReturnedCaptiveCount = ReturnCaptivesFromActiveDungeon();

	UE_LOG(LogTemp, Log, TEXT("[DungeonRuntime] PageId=%d returned before collapse with %d captive(s)"),
		ReturningPage->GetPageEntityId(), ReturnedCaptiveCount);
	return true;
}

int32 UWS_DungeonRuntime::ReturnCaptivesFromActiveDungeon()
{
	if (!GetWorld())
	{
		return 0;
	}

	int32 ReturnedCount = 0;
	for (TActorIterator<APageCharacter> It(GetWorld()); It; ++It)
	{
		APageCharacter* Captive = *It;
		if (!Captive || !Captive->IsCaptive() || !IsPageInActiveDungeon(Captive))
		{
			continue;
		}

		// Spread arrivals around the return point so multiple captives do not overlap.
		const int32 Column = ReturnedCount % 3;
		const int32 Row = ReturnedCount / 3;
		FTransform CaptiveReturnTransform = ActiveSession.ReturnTransform;
		CaptiveReturnTransform.AddToTranslation(FVector((Column - 1) * 130.f, 180.f + Row * 130.f, 0.f));
		CaptiveReturnTransform = ResolveGroundedCharacterTransform(GetWorld(), CaptiveReturnTransform, Captive);

		Captive->SetActorLocationAndRotation(
			CaptiveReturnTransform.GetLocation(),
			CaptiveReturnTransform.GetRotation().Rotator(),
			false,
			nullptr,
			ETeleportType::TeleportPhysics);
		Captive->SetIsInDungeon(false);
		Captive->CurrentJobState = FPageJobState{};
		Captive->SetTurnCombatState(false, false);
		if (UCharacterMovementComponent* Movement = Captive->GetCharacterMovement())
		{
			Movement->StopMovementImmediately();
			Movement->DisableMovement();
		}

		++ReturnedCount;
	}

	return ReturnedCount;
}

void UWS_DungeonRuntime::HandleDungeonCollapseExpired()
{
	if (!HasActiveDungeon())
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[DungeonRuntime] Dungeon collapse expired for PortalId=%d"), ActiveSession.PortalId);
	DestroyPagesStillInDungeon();
	EndActiveDungeonSession();
}

void UWS_DungeonRuntime::DestroyPagesStillInDungeon()
{
	for (TActorIterator<APageCharacter> It(GetWorld()); It; ++It)
	{
		APageCharacter* Page = *It;
		if (!Page || !IsPageInActiveDungeon(Page))
		{
			continue;
		}

		UE_LOG(LogTemp, Warning, TEXT("[DungeonRuntime] PageId=%d was lost in dungeon collapse"), Page->GetPageEntityId());
		Page->Destroy();
	}
}

void UWS_DungeonRuntime::EndActiveDungeonSession()
{
	if (UWS_PortalDirector* PortalDirector = GetWorld() ? GetWorld()->GetSubsystem<UWS_PortalDirector>() : nullptr)
	{
		PortalDirector->OnDungeonCleared(ActiveSession.PortalId);
	}

	ResetActiveSession();
}

void UWS_DungeonRuntime::MovePageIntoDungeon(APageCharacter* Page, const FTransform& EntryTransform)
{
	if (!Page)
	{
		return;
	}

	const FTransform FormationTransform = GetExpeditionFormationTransform(EntryTransform, Page);
	const FTransform GroundedTransform = ResolveGroundedCharacterTransform(GetWorld(), FormationTransform, Page);
	Page->SetActorLocationAndRotation(
		GroundedTransform.GetLocation(),
		GroundedTransform.GetRotation().Rotator(),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	Page->SetIsInDungeon(true);
	UE_LOG(LogTemp, Log, TEXT("[DungeonRuntime] PageId=%d entered expedition PortalId=%d at %s"),
		Page->GetPageEntityId(), ActiveSession.PortalId, *GroundedTransform.GetLocation().ToString());
}

bool UWS_DungeonRuntime::AddPageToActiveDungeon(APageCharacter* Page)
{
	if (!Page ||
		(ActiveSession.bCoreDestroyed && !IsDungeonCollapseActive()) ||
		!CanJoinActiveExpedition(Page))
	{
		return false;
	}

	const bool bAlreadyTracked = ActiveSession.DungeonPages.ContainsByPredicate(
		[Page](const TWeakObjectPtr<APageCharacter>& Candidate)
		{
			return Candidate.Get() == Page;
		});
	if (!bAlreadyTracked)
	{
		ActiveSession.DungeonPages.Add(Page);
	}

	if (ActiveSession.bPageTransferred)
	{
		MovePageIntoDungeon(Page, ActiveSession.EntryTransform);
	}
	return true;
}

FTransform UWS_DungeonRuntime::GetExpeditionFormationTransform(const FTransform& AnchorTransform, const APageCharacter* Page) const
{
	static const FVector FormationOffsets[] =
	{
		FVector::ZeroVector,
		FVector(140.f, 0.f, 0.f),
		FVector(-140.f, 0.f, 0.f),
		FVector(0.f, 140.f, 0.f),
		FVector(0.f, -140.f, 0.f),
		FVector(140.f, 140.f, 0.f),
		FVector(-140.f, 140.f, 0.f),
		FVector(140.f, -140.f, 0.f),
		FVector(-140.f, -140.f, 0.f)
	};

	int32 PageIndex = ActiveSession.DungeonPages.IndexOfByPredicate(
		[Page](const TWeakObjectPtr<APageCharacter>& Candidate)
		{
			return Candidate.Get() == Page;
		});
	PageIndex = FMath::Max(0, PageIndex);

	FTransform Result = AnchorTransform;
	const int32 OffsetIndex = PageIndex % UE_ARRAY_COUNT(FormationOffsets);
	const int32 Ring = PageIndex / UE_ARRAY_COUNT(FormationOffsets);
	const FVector LocalOffset = FormationOffsets[OffsetIndex] * (1.f + Ring);
	Result.AddToTranslation(AnchorTransform.TransformVectorNoScale(LocalOffset));
	return Result;
}

bool UWS_DungeonRuntime::CanJoinActiveExpedition(const APageCharacter* Page) const
{
	if (!Page || !Page->IsFriendly())
	{
		return false;
	}

	const UStatsComponent* Stats = Page->GetStats();
	return !Stats || (!Stats->IsDead() && !Stats->IsDowned() && !Stats->IsRecovering());
}




