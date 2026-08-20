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

	/** Per-Page, per-skill aptitude applied to all XP gained for this skill. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.01"))
	float ExperienceGainTalent = 1.f;

	/** Per-Page, per-skill aptitude applied to this skill's gameplay multiplier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.01"))
	float EffectTalent = 1.f;

	/** Distinguishes legacy save states from a deliberately neutral 1.0 talent. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bTalentInitialized = false;
};
