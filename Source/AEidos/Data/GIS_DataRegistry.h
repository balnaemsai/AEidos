// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Definitions/PortalDefinitionRow.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/StreamableManager.h"
#include "GIS_DataRegistry.generated.h"

class UEidosDataRegistryConfig;
class UEidosDataRegistrySettings;
class UDataTable;
struct FSkillDefinitionRow;
struct FWorkDefinitionRow;
struct FBuildingDefinitionRow;
struct FResourceDefinitionRow;
struct FItemDefinitionRow;
struct FBlockDefinitionRow;
struct FBlockInteractionDefinitionRow;
struct FResearchDefinitionRow;

DECLARE_LOG_CATEGORY_EXTERN(LogDataRegistry, Log, All);

/**
 * 
 */
UCLASS()
class AEIDOS_API UGIS_DataRegistry : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
	public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	bool IsReady() const {return bReady;}
	bool IsLoading() const {return bLoading;}
	const FString& GetNotReadyReason() const {return NotReadyReason;}
	
	void EnsureReady(TFunction<void(bool bSuccess)> OnReady);

	bool EnsureReadySync();

	UDataTable* FindDataTableByName(const FName TableName) const;

	UDataTable* GetSkillTable() const;
	const FSkillDefinitionRow* GetSkillDef(FName SkillId) const;

	UDataTable* GetWorkTable() const;
	const FWorkDefinitionRow* GetWorkDef(FName SkillId) const;

	UDataTable* GetResearchTable() const;
	const FResearchDefinitionRow* GetResearchDef(FName ResearchId) const;

	UDataTable* GetBuildingTable() const;
	const FBuildingDefinitionRow* GetBuildingDef(FName BuildingId) const;

	UDataTable* GetResourceTable() const;
	const FResourceDefinitionRow* GetResourceDef(FName ResourceId) const;
	TArray<FName> GetAllResourceIds() const;

	UDataTable* GetItemTable() const;
	const FItemDefinitionRow* GetItemDef(FName ItemId) const;
	TArray<FName> GetAllItemIds() const;

	/** CSV-backed world block state definitions. */
	UDataTable* GetBlockTable() const;
	const FBlockDefinitionRow* GetBlockDef(FName BlockId) const;

	/** CSV-backed actions available for each world block. */
	UDataTable* GetBlockInteractionTable() const;
	void GetBlockInteractions(FName BlockId, TArray<const FBlockInteractionDefinitionRow*>& OutInteractions) const;

	UDataTable* GetPortalTable() const;
	const FPortalDefinitionRow* GetPortalDef(FName PortalDefId) const;
	TArray<FName> GetAllPortalDefIds() const;

	private:
	TSoftObjectPtr<UEidosDataRegistryConfig> ConfigRef;
	UPROPERTY(Transient)
	TObjectPtr<UEidosDataRegistryConfig> LoadedConfig = nullptr;

	bool bReady = false;
	bool bLoading = false;
	FString NotReadyReason;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UDataTable>> LoadedTables;

	TArray<TFunction<void(bool)>> PendingCallbacks;

	TSharedPtr<FStreamableHandle> ActiveHandle;
	FStreamableManager& GetStreamable() const;

	void BeginLoad_ConfigThenAssets();
	void OnConfigLoaded();
	void BeginLoad_RequiredAssets();
	void OnAssetsLoaded();

	bool ValidateConfig();
	bool CacheLoadedAssets();
	void BroadcastReady(bool bSuccess);

	const UEidosDataRegistrySettings* GetSettings() const;
	
};
