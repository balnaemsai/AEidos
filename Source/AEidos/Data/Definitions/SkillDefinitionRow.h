#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "SkillDefinitionRow.generated.h"

USTRUCT(BlueprintType)
struct FSkillDefinitionRow : public FTableRowBase
{
	GENERATED_BODY()

	// RowName과 동일하게 맞춰 쓰는 것을 권장
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName SkillId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	// 레벨 0 -> 1 기준 필요 경험치
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float BaseExpToLevel = 100.f;

	// 다음 레벨 요구치 증가 계수
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float ExpGrowthFactor = 1.12f;

	// 레벨당 성능 배율 증가량
	// 예: 0.05면 레벨 1당 5% 증가
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float MultiplierPerLevel = 0.05f;

	// 스킬 최대 레벨
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 MaxLevel = 100;

	// 연관 스킬 전파
	// 예: Running +10 XP -> Endurance +1 XP (계수 0.1)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<FName, float> RelatedSkillCoefficients;
};