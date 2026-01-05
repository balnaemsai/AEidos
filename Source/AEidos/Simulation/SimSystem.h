#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SimSystem.generated.h"

class USimCommandBuffer;

UINTERFACE()
class AEIDOS_API USimSystem : public UInterface
{
	GENERATED_BODY()
};

class AEIDOS_API ISimSystem
{
	GENERATED_BODY()

public:
	// 실행 순서(작을수록 먼저). 필요 없으면 전부 0으로 두고 나중에 확장.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Eidos|Simulation")
	int32 GetSimOrder() const;

	// Plan: 변경은 CommandBuffer에 enqueue (월드 직접 변경 지양)
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Eidos|Simulation")
	void SimPlan(USimCommandBuffer* Cmd, float FixedDeltaSeconds);

	// Post: Commit 이후 후처리
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Eidos|Simulation")
	void SimPost(float FixedDeltaSeconds);
};
