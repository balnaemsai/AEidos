// Fill out your copyright notice in the Description page of Project Settings.


#include "Data/GIS_DataRegistry.h"
#include "Engine/AssetManager.h"
#include "Engine/DataTable.h"
#include "Settings/EidosDataRegistrySettings.h"
#include "Entities/Page/Components/SkillDefRow.h"
#include "Data/EidosDataRegistryConfig.h"
#include "World/Settlement/WorkDefinitionRow.h"

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

	// Required 목록 동기 로드
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
		// 데이터가 없는데도 Ready로 둘지 정책 선택:
		// 지금은 "비어 있으면 실패"가 디버깅에 유리.
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
	// DT asset 이름이 DT_Skill 이라는 전제
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
	// DT asset 이름이 DT_Skill 이라는 전제
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









