// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SkillStruct.h"
#include "SkillComponent.generated.h"

struct FSkillDefinitionRow;
class UGIS_DataRegistry;


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class AEIDOS_API USkillComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	USkillComponent();

	// 일반 XP 지급 (연관 스킬 전파 포함)
	UFUNCTION(BlueprintCallable, Category="Skill")
	void AddSkillXP(FName SkillId, float Amount);

	// 연관 스킬 전파 없는 직접 지급
	UFUNCTION(BlueprintCallable, Category="Skill")
	void AddSkillXP_NoPropagation(FName SkillId, float Amount);

	// 시간 기반 행동 XP
	UFUNCTION(BlueprintCallable, Category="Skill")
	void AddContinuousSkillXP(FName SkillId, float RatePerSecond, float DeltaSeconds, float XPFactor = 1.f);

	// 이벤트성 행동 XP
	UFUNCTION(BlueprintCallable, Category="Skill")
	void AddDiscreteSkillXP(FName SkillId, float FlatXP);

	UFUNCTION(BlueprintPure, Category="Skill")
	int32 GetSkillLevel(FName SkillId) const;

	UFUNCTION(BlueprintPure, Category="Skill")
	float GetSkillXP(FName SkillId) const;

	UFUNCTION(BlueprintPure, Category="Skill")
	float GetSkillMultiplier(FName SkillId) const;

	UFUNCTION(BlueprintPure, Category="Skill")
	bool HasSkill(FName SkillId) const;

	const TMap<FName, FPageSkillRuntime>& GetAllSkillStates() const { return Skills; }

	// Save/Load 용
	void SetAllSkillStates(const TMap<FName, FPageSkillRuntime>& InStates);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Skill")
	TMap<FName, FPageSkillRuntime> Skills;

private:
	void AddSkillXP_Internal(FName SkillId, float Amount, bool bAllowPropagation);

	void EnsureSkillExists(FName SkillId);

	const FSkillDefinitionRow* GetSkillDef(FName SkillId) const;

	int32 EvaluateLevelFromXP(const FSkillDefinitionRow& Def, float InXP) const;
	float GetRequiredXPForLevel(const FSkillDefinitionRow& Def, int32 Level) const;

		
};
