// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/StreamableManager.h"
#include "GIS_DataRegistry.generated.h"

class UEidosDataRegistryConfig;
class UEidosDataRegistrySettings;

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
