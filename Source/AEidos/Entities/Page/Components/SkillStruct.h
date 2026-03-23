#pragma once

#include "CoreMinimal.h"
#include "SkillStruct.generated.h"

USTRUCT(BlueprintType)
struct FPageSkillRuntime
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName SkillId;

	// 누적 총 경험치
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TotalXP = 0.f;

	// 현재 레벨
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Level = 0;
};