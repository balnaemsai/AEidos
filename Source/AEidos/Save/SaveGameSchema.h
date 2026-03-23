#pragma once

#include "CoreMinimal.h"
#include "World/Settlement/WorkTypes.h"
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

	// Actor/WS들이 자유롭게 저장할 수 있는 "키-값 저장소" (간단하고 유연)
	// 예) "Settlement.TotalWood" -> "123"
	UPROPERTY(BlueprintReadWrite)
	TMap<FName, FString> KV;

	UPROPERTY(BlueprintReadWrite)
	FEidosWorkSnapshot Work;

	// Helper
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


