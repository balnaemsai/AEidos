// Fill out your copyright notice in the Description page of Project Settings.


#include "Data/GIS_DataRegistry.h"
#include "Engine/AssetManager.h"
#include "Engine/DataTable.h"
#include "Settings/EidosDataRegistrySettings.h"
#include "Data/Definitions/SkillDefinitionRow.h"
#include "Data/EidosDataRegistryConfig.h"
#include "Data/Definitions/WorkDefinitionRow.h"
#include "Data/Definitions/BuildingDefinitionRow.h"
#include "Data/Definitions/ResourceDefinitionRow.h"
#include "Data/Definitions/ItemDefinitionRow.h"
#include "Data/Definitions/BlockDefinitionRow.h"
#include "Data/Definitions/BlockInteractionDefinitionRow.h"
#include "Data/Definitions/ResearchDefinitionRow.h"
#include "Data/Definitions/DungeonAttributeDefinitionRow.h"
#include "Data/Definitions/ScenarioDefinitionRow.h"

DEFINE_LOG_CATEGORY(LogDataRegistry);

void UGIS_DataRegistry::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	bReady = false;
	bLoading = false;
	NotReadyReason.Empty();
	LoadedTables.Empty();
	PendingCallbacks.Empty();

	const UEidosDataRegistrySettings* S = GetSettings();
	if (S)
	{
		ConfigRef = S->RegistryConfig;
	}
}

void UGIS_DataRegistry::Deinitialize()
{
	ActiveHandle.Reset();
	PendingCallbacks.Empty();
	LoadedTables.Empty();
	LoadedConfig = nullptr;

	Super::Deinitialize();
}

void UGIS_DataRegistry::EnsureReady(TFunction<void(bool)> OnReady)
{
	if (bReady)
	{
		OnReady(true);
		return;
	}

	PendingCallbacks.Add(MoveTemp(OnReady));

	if (bLoading)
	{
		return;
	}

	BeginLoad_ConfigThenAssets();
}

bool UGIS_DataRegistry::EnsureReadySync()
{
	if (bReady)
	{
		return true;
	}

	const UEidosDataRegistrySettings* S = GetSettings();
	if (!S)
	{
		NotReadyReason = TEXT("EidosDataRegistrySettings missing.");
		return false;
	}

	if (ConfigRef.IsNull())
	{
		NotReadyReason = TEXT("RegistryConfig not set in Project Settings.");
		return false;
	}

	LoadedConfig = ConfigRef.LoadSynchronous();
	if (!LoadedConfig)
	{
		NotReadyReason = TEXT("Failed to LoadSynchronous RegistryConfig.");
		return false;
	}

	// Required 紐⑸줉 ?숆린 濡쒕뱶
	for (const auto& TRef : LoadedConfig->RequiredDataTables)
	{
		if (TRef.IsNull()) continue;
		TRef.LoadSynchronous();
	}
	for (const auto& ARef : LoadedConfig->RequiredPrimaryAssets)
	{
		if (ARef.IsNull()) continue;
		ARef.LoadSynchronous();
	}
	for (const auto& ORef : LoadedConfig->OptionalAssets)
	{
		if (ORef.IsNull()) continue;
		ORef.LoadSynchronous();
	}

	const bool bOk = CacheLoadedAssets();
	bReady = bOk;
	bLoading = false;
	return bOk;
}

void UGIS_DataRegistry::BeginLoad_ConfigThenAssets()
{
	bLoading = true;
	bReady = false;
	NotReadyReason.Empty();

	if (ConfigRef.IsNull())
	{
		NotReadyReason = TEXT("RegistryConfig not set in Project Settings (Eidos Data Registry).");
		UE_LOG(LogDataRegistry, Error, TEXT("%s"), *NotReadyReason);
		BroadcastReady(false);
		return;
	}

	TArray<FSoftObjectPath> Paths;
	Paths.Add(ConfigRef.ToSoftObjectPath());

	UE_LOG(LogDataRegistry, Log, TEXT("[DataRegistry] Loading RegistryConfig..."));

	ActiveHandle = GetStreamable().RequestAsyncLoad(Paths,FStreamableDelegate::CreateUObject(this, &UGIS_DataRegistry::OnConfigLoaded), FStreamableManager::AsyncLoadHighPriority);
}

void UGIS_DataRegistry::OnConfigLoaded()
{
	LoadedConfig = ConfigRef.Get();

	if (!LoadedConfig)
	{
		NotReadyReason = TEXT("RegistryConfig loaded callback but LoadedConfig is null.");
		UE_LOG(LogDataRegistry, Error, TEXT("%s"), *NotReadyReason);
		BroadcastReady(false);
		return;
	}

	if (!ValidateConfig())
	{
		UE_LOG(LogDataRegistry, Error, TEXT("%s"), *NotReadyReason);
		BroadcastReady(false);
		return;
	}

	BeginLoad_RequiredAssets();
}

void UGIS_DataRegistry::BeginLoad_RequiredAssets()
{
	TArray<FSoftObjectPath> Paths;

	auto AddAll = [&Paths](const auto& Arr)
	{
		for (const auto&Ref : Arr)
		{
			if (!Ref.IsNull())
			{
				Paths.Add(Ref.ToSoftObjectPath());
			}
		}
	};

	AddAll(LoadedConfig->RequiredDataTables);
	AddAll(LoadedConfig->RequiredPrimaryAssets);
	AddAll(LoadedConfig->OptionalAssets);

	if (Paths.Num() == 0)
	{
		NotReadyReason = TEXT("RegistryConfig has no assets to load.");
		UE_LOG(LogDataRegistry, Warning, TEXT("%s"), *NotReadyReason);
		// ?곗씠?곌? ?녿뒗?곕룄 Ready濡??섏? ?뺤콉 ?좏깮:
		// 吏湲덉? "鍮꾩뼱 ?덉쑝硫??ㅽ뙣"媛 ?붾쾭源낆뿉 ?좊━.
		BroadcastReady(false);
		return;
	}

	UE_LOG(LogDataRegistry, Log, TEXT("[DataRegistry] Loading %d required assets..."), Paths.Num());

	ActiveHandle = GetStreamable().RequestAsyncLoad(
		Paths,
		FStreamableDelegate::CreateUObject(this, &UGIS_DataRegistry::OnAssetsLoaded),
		FStreamableManager::AsyncLoadHighPriority
	);
}

void UGIS_DataRegistry::OnAssetsLoaded()
{
	const bool bOk = CacheLoadedAssets();
	bReady = bOk;
	bLoading = false;

	if (!bOk && NotReadyReason.IsEmpty())
	{
		NotReadyReason = TEXT("Assets loaded but caching/validation failed.");
	}

	BroadcastReady(bOk);
}

bool UGIS_DataRegistry::ValidateConfig()
{
	if (LoadedConfig->RequiredDataTables.Num() == 0)
	{
		NotReadyReason = TEXT("RegistryConfig RequiredDataTables is empty (set at least ResourceTable).");
		return false;
	}

	for (const auto& TRef : LoadedConfig->RequiredDataTables)
	{
		if (TRef.IsNull())
		{
			NotReadyReason = TEXT("RegistryConfig has null entry in RequiredDataTables.");
			return false;
		}
	}
	return true;
}

bool UGIS_DataRegistry::CacheLoadedAssets()
{
	LoadedTables.Empty();

	if (!LoadedConfig)
	{
		NotReadyReason = TEXT("CacheLoadedAssets: LoadedConfig is null.");
		return false;
	}

	for (const auto& TRef : LoadedConfig->RequiredDataTables)
	{
		UDataTable* DT = TRef.Get();
		if (!DT)
		{
			NotReadyReason = FString::Printf(TEXT("Required DataTable failed to load: %s"), *TRef.ToString());
			UE_LOG(LogDataRegistry, Error, TEXT("%s"), *NotReadyReason);
			return false;
		}

		const FName Key = DT->GetFName();
		LoadedTables.Add(Key, DT);
	}

	UE_LOG(LogDataRegistry, Log, TEXT("[DataRegistry] Ready. LoadedTables=%d"), LoadedTables.Num());
	return true;
}

void UGIS_DataRegistry::BroadcastReady(bool bSuccess)
{
	ActiveHandle.Reset();

	TArray<TFunction<void(bool)>> Callbacks = MoveTemp(PendingCallbacks);
	PendingCallbacks.Reset();

	for (auto& Fn : Callbacks)
	{
		Fn(bSuccess);
	}
}

UDataTable* UGIS_DataRegistry::FindDataTableByName(const FName TableName) const
{
	if (const TObjectPtr<UDataTable>* Found = LoadedTables.Find(TableName))
	{
		return Found->Get();
	}
	return nullptr;
}

UDataTable* UGIS_DataRegistry::GetSkillTable() const
{
	// DT asset ?대쫫??DT_Skill ?대씪???꾩젣
	return FindDataTableByName(TEXT("DT_Skill"));
}

FStreamableManager& UGIS_DataRegistry::GetStreamable() const
{
	return UAssetManager::GetStreamableManager();
}

const UEidosDataRegistrySettings* UGIS_DataRegistry::GetSettings() const
{
	return GetDefault<UEidosDataRegistrySettings>();
}

const FSkillDefinitionRow* UGIS_DataRegistry::GetSkillDef(FName SkillId) const
{
	UDataTable* SkillTable = GetSkillTable();
	if (!SkillTable || SkillId.IsNone())
	{
		return nullptr;
	}

	return SkillTable->FindRow<FSkillDefinitionRow>(SkillId, TEXT("GetSkillDef"));
}

UDataTable* UGIS_DataRegistry::GetWorkTable() const
{
	// DT asset ?대쫫??DT_Skill ?대씪???꾩젣
	return FindDataTableByName(TEXT("DT_Work"));
}

const FWorkDefinitionRow* UGIS_DataRegistry::GetWorkDef(FName WorkId) const
{
	UDataTable* WorkTable = GetWorkTable();
	if (!WorkTable || WorkId.IsNone())
	{
		return nullptr;
	}

	return WorkTable->FindRow<FWorkDefinitionRow>(WorkId, TEXT("GetWorkDef"));
}

UDataTable* UGIS_DataRegistry::GetBuildingTable() const
{
	return FindDataTableByName(TEXT("DT_Building"));
}

const FBuildingDefinitionRow* UGIS_DataRegistry::GetBuildingDef(FName BuildingId) const
{
	UDataTable* Table = GetBuildingTable();
	if (!Table || BuildingId.IsNone())
	{
		UE_LOG(LogTemp, Error, TEXT("[DataRegistry] No Table or BuildingId is None"));
		return nullptr;
	}

	return Table->FindRow<FBuildingDefinitionRow>(BuildingId, TEXT("GetBuildingDef"));
}

UDataTable* UGIS_DataRegistry::GetResourceTable() const
{
	return FindDataTableByName(TEXT("DT_Resource"));
}

const FResourceDefinitionRow* UGIS_DataRegistry::GetResourceDef(FName ResourceId) const
{
	UDataTable* ResourceTable = GetResourceTable();
	if (!ResourceTable || ResourceId.IsNone())
	{
		UE_LOG(LogTemp, Error, TEXT("[DataRegistry] No ResourceId is None"));
		return nullptr;
	}

	return ResourceTable->FindRow<FResourceDefinitionRow>(ResourceId, TEXT("GetResourceDef"));
}

TArray<FName> UGIS_DataRegistry::GetAllResourceIds() const
{
	TArray<FName> Result;

	UDataTable* Table = GetResourceTable();
	if (!Table)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DataRegistry] DT_Resource not found"));
		return Result;
	}

	Result = Table->GetRowNames();
	return Result;
}

UDataTable* UGIS_DataRegistry::GetPortalTable() const
{
	if (UDataTable* Table = FindDataTableByName(TEXT("DT_Portal")))
	{
		return Table;
	}

	return FindDataTableByName(TEXT("DT_Portalfix"));
}

const FPortalDefinitionRow* UGIS_DataRegistry::GetPortalDef(FName PortalDefId) const
{
	UDataTable* Table = GetPortalTable();
	if (!Table || PortalDefId.IsNone())
	{
		UE_LOG(LogTemp, Error, TEXT("[DataRegistry] No Portal Table or PortalDefId is None"));
		return nullptr;
	}

	return Table->FindRow<FPortalDefinitionRow>(PortalDefId, TEXT("GetPortalDef"));
}

UDataTable* UGIS_DataRegistry::GetScenarioTable() const
{
	return FindDataTableByName(TEXT("DT_Scenario"));
}

const FScenarioDefinitionRow* UGIS_DataRegistry::GetScenarioDef(FName ScenarioId) const
{
	UDataTable* Table = GetScenarioTable();
	return Table && !ScenarioId.IsNone()
		? Table->FindRow<FScenarioDefinitionRow>(ScenarioId, TEXT("GetScenarioDef"))
		: nullptr;
}

UDataTable* UGIS_DataRegistry::GetResearchTable() const
{
	return FindDataTableByName(TEXT("DT_Research"));
}

const FResearchDefinitionRow* UGIS_DataRegistry::GetResearchDef(FName ResearchId) const
{
	UDataTable* ResearchTable = GetResearchTable();
	return ResearchTable && !ResearchId.IsNone()
		? ResearchTable->FindRow<FResearchDefinitionRow>(ResearchId, TEXT("GetResearchDef"))
		: nullptr;
}

UDataTable* UGIS_DataRegistry::GetItemTable() const
{
	return FindDataTableByName(TEXT("DT_Item"));
}

const FItemDefinitionRow* UGIS_DataRegistry::GetItemDef(FName ItemId) const
{
	UDataTable* ItemTable = GetItemTable();
	if (!ItemTable || ItemId.IsNone())
	{
		return nullptr;
	}

	return ItemTable->FindRow<FItemDefinitionRow>(ItemId, TEXT("GetItemDef"));
}

TArray<FName> UGIS_DataRegistry::GetAllItemIds() const
{
	UDataTable* ItemTable = GetItemTable();
	return ItemTable ? ItemTable->GetRowNames() : TArray<FName>{};
}

UDataTable* UGIS_DataRegistry::GetBlockTable() const
{
	return FindDataTableByName(TEXT("DT_Block"));
}

const FBlockDefinitionRow* UGIS_DataRegistry::GetBlockDef(FName BlockId) const
{
	UDataTable* BlockTable = GetBlockTable();
	return BlockTable && !BlockId.IsNone()
		? BlockTable->FindRow<FBlockDefinitionRow>(BlockId, TEXT("GetBlockDef"))
		: nullptr;
}

UDataTable* UGIS_DataRegistry::GetBlockInteractionTable() const
{
	return FindDataTableByName(TEXT("DT_BlockInteraction"));
}

void UGIS_DataRegistry::GetBlockInteractions(FName BlockId,
	TArray<const FBlockInteractionDefinitionRow*>& OutInteractions) const
{
	OutInteractions.Reset();
	UDataTable* InteractionTable = GetBlockInteractionTable();
	if (!InteractionTable || BlockId.IsNone())
	{
		return;
	}

	for (const TPair<FName, uint8*>& Pair : InteractionTable->GetRowMap())
	{
		const FBlockInteractionDefinitionRow* Interaction =
			reinterpret_cast<const FBlockInteractionDefinitionRow*>(Pair.Value);
		if (Interaction && Interaction->BlockId == BlockId && !Interaction->InteractionId.IsNone())
		{
			OutInteractions.Add(Interaction);
		}
	}
}

TArray<FName> UGIS_DataRegistry::GetAllPortalDefIds() const
{
	TArray<FName> Result;

	UDataTable* Table = GetPortalTable();
	if (!Table)
	{
		return Result;
	}

	Result = Table->GetRowNames();
	return Result;
}

UDataTable* UGIS_DataRegistry::GetDungeonAttributeTable() const
{
	return FindDataTableByName(TEXT("DT_DungeonAttribute"));
}

const FDungeonAttributeDefinitionRow* UGIS_DataRegistry::GetDungeonAttributeDef(FName AttributeId) const
{
	UDataTable* Table = GetDungeonAttributeTable();
	return Table && !AttributeId.IsNone()
		? Table->FindRow<FDungeonAttributeDefinitionRow>(AttributeId, TEXT("GetDungeonAttributeDef"))
		: nullptr;
}

void UGIS_DataRegistry::GetEligibleDungeonAttributes(float Difficulty, TArray<const FDungeonAttributeDefinitionRow*>& OutAttributes) const
{
	OutAttributes.Reset();
	UDataTable* Table = GetDungeonAttributeTable();
	if (!Table)
	{
		return;
	}

	for (const TPair<FName, uint8*>& Pair : Table->GetRowMap())
	{
		const FDungeonAttributeDefinitionRow* Row = reinterpret_cast<const FDungeonAttributeDefinitionRow*>(Pair.Value);
		if (Row && !Row->AttributeId.IsNone() && !Row->CoreShardItemId.IsNone()
			&& Row->SelectionWeight > 0.f && Difficulty >= Row->MinimumDifficulty && Difficulty <= Row->MaximumDifficulty)
		{
			OutAttributes.Add(Row);
		}
	}
}

void UGIS_DataRegistry::GetEligiblePortalDungeonPresets(float Difficulty, TArray<const FPortalDefinitionRow*>& OutPresets) const
{
	OutPresets.Reset();
	UDataTable* Table = GetPortalTable();
	if (!Table)
	{
		return;
	}

	for (const TPair<FName, uint8*>& Pair : Table->GetRowMap())
	{
		const FPortalDefinitionRow* Row = reinterpret_cast<const FPortalDefinitionRow*>(Pair.Value);
		if (Row && !Row->PortalId.IsNone() && !Row->PresetAsset.IsNull()
			&& Row->SelectionWeight > 0.f && Difficulty >= Row->MinimumDifficulty && Difficulty <= Row->MaximumDifficulty)
		{
			OutPresets.Add(Row);
		}
	}
}









