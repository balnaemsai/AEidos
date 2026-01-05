// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/DataTable.h"
#include "GIS_DataRegistry.generated.h"

/**
 * 
 */
UCLASS()
class AEIDOS_API UGIS_DataRegistry : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
	public:
	/*
	virtual void Initalize(FSubsystemCollectionBase* Collection) override;
	virtual void Deinitialize() override;
	
	bool IsReady() const {return bReady;}
	bool IsLoading() const {return bLoading;}
	
	void EnsureReady(TFunction<void(bool)> OnReady);
	
	UDataTable* GetResourceTable() const {return ResourceTable;}
	UDataTable* GetBuildingTable() const {return BuildingTable;}
	UDataTable* GetDungeonTable() const {return DungeonTable;}
	
	FString GetNotReadyReason() const {return NotReadyReason;}
	
	private:
	UPROPERTY(EditDefaultsOnly)
	
	*/
	
};
