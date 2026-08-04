#pragma once

#include "CoreMinimal.h"
#include "Core/Types/ItemTypes.h"
#include "Core/Types/WorkTypes.h"
#include "SaveGameSchema.generated.h"

USTRUCT(BlueprintType)
struct FEidosWorkSnapshot
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite)
	int32 NextRequestId = 1;

	UPROPERTY(BlueprintReadWrite)
	int32 NextInstanceId = 1;

	UPROPERTY(BlueprintReadWrite)
	TArray<FWorkRequest> Queue;

	UPROPERTY(BlueprintReadWrite)
	TArray<FWorkInstance> ActiveInstances;

	// PageJobs는 런타임 캐시라 저장 안 함(권장)
};

USTRUCT(BlueprintType)
struct FEidosGameSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FString Mode;

	UPROPERTY(BlueprintReadWrite)
	int32 WorldSeed = 0;
};

USTRUCT(BlueprintType)
struct FEidosEconomySnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	TMap<FName, int32> ResourceAmounts;
};

USTRUCT(BlueprintType)
struct FEidosSustenanceSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	float StoredMealUnits = 0.f;

	UPROPERTY(BlueprintReadWrite)
	float StoredMealQualityTotal = 0.f;

	UPROPERTY(BlueprintReadWrite)
	float LastServedAverageMealQuality = 0.f;

	UPROPERTY(BlueprintReadWrite)
	float CurrentDailyMealDemandUnits = 0.f;

	UPROPERTY(BlueprintReadWrite)
	int32 LastKnownPopulation = 0;
};

USTRUCT(BlueprintType)
struct FEidosPageSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	int32 PageId = 0;

	UPROPERTY(BlueprintReadWrite)
	FTransform Transform = FTransform::Identity;

	UPROPERTY(BlueprintReadWrite)
	FSoftClassPath PageClass;

	UPROPERTY(BlueprintReadWrite)
	TArray<FItemStack> InventoryStacks;

	UPROPERTY(BlueprintReadWrite)
	TArray<FPageEquipmentSlotState> EquipmentSlots;
};

USTRUCT(BlueprintType)
struct FEidosPopulationSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	int32 NextPageId = 1;

	UPROPERTY(BlueprintReadWrite)
	TArray<FEidosPageSnapshot> Pages;
};

UENUM(BlueprintType)
enum class EConstructionSiteLifecycle : uint8
{
	Queued,
	InProgress,
	AwaitingFinalization,
	Completed,
	Cancelled,
	Failed
};

USTRUCT(BlueprintType)
struct FEidosConstructionSiteSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	int32 SiteId = 0;

	UPROPERTY(BlueprintReadWrite)
	FName BuildingId;

	UPROPERTY(BlueprintReadWrite)
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite)
	float YawDeg = 0.f;

	UPROPERTY(BlueprintReadWrite)
	int32 WorkRequestId = 0;

	UPROPERTY(BlueprintReadWrite)
	EConstructionSiteLifecycle State = EConstructionSiteLifecycle::Queued;
};

USTRUCT(BlueprintType)
struct FEidosBuildingSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	int32 NextSiteId = 1;

	UPROPERTY(BlueprintReadWrite)
	TArray<FEidosConstructionSiteSnapshot> Sites;
};

USTRUCT(BlueprintType)
struct FEidosWorldSnapshot
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite)
	int32 SchemaVersion = 1;

	UPROPERTY(BlueprintReadWrite)
	FString MapName;

	// 생성 시각/시간(원하면 게임시간/일수 등 추가)
	UPROPERTY(BlueprintReadWrite)
	FDateTime SavedAtUtc;

	UPROPERTY(BlueprintReadWrite)
	FEidosGameSnapshot Game;

	UPROPERTY(BlueprintReadWrite)
	FEidosEconomySnapshot Economy;

	UPROPERTY(BlueprintReadWrite)
	FEidosSustenanceSnapshot Sustenance;

	UPROPERTY(BlueprintReadWrite)
	FEidosPopulationSnapshot Population;

	UPROPERTY(BlueprintReadWrite)
	FEidosBuildingSnapshot Buildings;

	UPROPERTY(BlueprintReadWrite)
	FEidosWorkSnapshot Work;

	UPROPERTY(BlueprintReadWrite)
	TMap<FName, FString> KV;

	// Optional extension/debug storage. Core gameplay state should prefer typed snapshots.
	FString GetKVString(FName Key, const FString& DefaultValue = TEXT("")) const
	{
		if (const FString* Found = KV.Find(Key))
		{
			return *Found;
		}
		return DefaultValue;
	}

	void SetKVString(FName Key, const FString& Value)
	{
		KV.Add(Key, Value);
	}
};


