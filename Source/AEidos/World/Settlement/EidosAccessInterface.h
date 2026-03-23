#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "EidosAccessInterface.generated.h"


UINTERFACE()
class AEIDOS_API UEidosEconomyAccess : public UInterface
{
	GENERATED_BODY()
};

class AEIDOS_API IEidosEconomyAccess
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	bool CanAfford(const TArray<FWorkCost>& Costs) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void ConsumeCosts(const TArray<FWorkCost>& Costs);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void GrantRewards(const TArray<FWorkReward>& Rewards);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	int32 GetResourceAmount(FName ResourceId) const;
};

UINTERFACE()
class AEIDOS_API UEidosPopulationAccess : public UInterface
{
	GENERATED_BODY()
};

class AEIDOS_API IEidosPopulationAccess
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	TArray<int32> GetAllPageIds() const;

	// 페이지가 지금 일할 수 있는 상태인지(기절/휴식/전투중 등)
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	bool IsPageAvailable(int32 PageId) const;

	// 작업장으로 이동 처리(간단히는 “목표 위치 설정”)
	//UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	//bool EnsureMoveTo(int32 PageId, const FVector& WorldLocation);

	// 이동 완료/작업 시작 가능 여부(거리 체크 등)
	//UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	//bool IsAtLocation(int32 PageId, const FVector& WorldLocation, float Tolerance) const;

	// 작업 속도 계산에 필요한 값들(스탯/스킬/특성/상태 반영)
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	float ComputeWorkRateMultiplier(int32 PageId, FName WorkId) const;

	// 완료 보상(스킬 경험치 등)
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void ApplyWorkCompletionEffects(int32 PageId, FName WorkId);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	AActor* GetPageActor(int32 PageId);
	
};
