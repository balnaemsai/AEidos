// Fill out your copyright notice in the Description page of Project Settings.


#include "Entities/Page/Components/SkillComponent.h"
#include "SkillDefRow.h"
#include "Data/GIS_DataRegistry.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

// Sets default values for this component's properties
USkillComponent::USkillComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
}

void USkillComponent::SetAllSkillStates(const TMap<FName, FPageSkillRuntime>& InStates)
{
	Skills = InStates;
}

bool USkillComponent::HasSkill(FName SkillId) const
{
	return Skills.Contains(SkillId);
}

void USkillComponent::EnsureSkillExists(FName SkillId)
{
	if (!Skills.Contains(SkillId))
	{
		FPageSkillRuntime NewState;
		NewState.SkillId = SkillId;
		NewState.TotalXP = 0.f;
		NewState.Level = 0;
		Skills.Add(SkillId, NewState);
	}
}

const FSkillDefinitionRow* USkillComponent::GetSkillDef(FName SkillId) const
{
	if (!GetWorld())
	{
		return nullptr;
	}

	UGameInstance* GI = GetWorld()->GetGameInstance();
	if (!GI)
	{
		return nullptr;
	}

	UGIS_DataRegistry* Registry = GI->GetSubsystem<UGIS_DataRegistry>();
	if (!Registry)
	{
		return nullptr;
	}

	return Registry->GetSkillDef(SkillId);
}

float USkillComponent::GetRequiredXPForLevel(const FSkillDefinitionRow& Def, int32 Level) const
{
	return Def.BaseExpToLevel * FMath::Pow(Def.ExpGrowthFactor, (float)Level);
}

int32 USkillComponent::EvaluateLevelFromXP(const FSkillDefinitionRow& Def, float InXP) const
{
	int32 Level = 0;
	float Remaining = InXP;

	while (Level < Def.MaxLevel)
	{
		const float Need = GetRequiredXPForLevel(Def, Level);
		if (Remaining >= Need)
		{
			Remaining -= Need;
			++Level;
		}
		else
		{
			break;
		}
	}

	return Level;
}

void USkillComponent::AddSkillXP(FName SkillId, float Amount)
{
	AddSkillXP_Internal(SkillId, Amount, true);
}

void USkillComponent::AddSkillXP_NoPropagation(FName SkillId, float Amount)
{
	AddSkillXP_Internal(SkillId, Amount, false);
}

void USkillComponent::AddContinuousSkillXP(FName SkillId, float RatePerSecond, float DeltaSeconds, float XPFactor)
{
	if (RatePerSecond <= 0.f || DeltaSeconds <= 0.f || XPFactor <= 0.f)
	{
		return;
	}

	AddSkillXP(SkillId, RatePerSecond * DeltaSeconds * XPFactor);
}

void USkillComponent::AddDiscreteSkillXP(FName SkillId, float FlatXP)
{
	if (FlatXP <= 0.f)
	{
		return;
	}

	AddSkillXP(SkillId, FlatXP);
}

void USkillComponent::AddSkillXP_Internal(FName SkillId, float Amount, bool bAllowPropagation)
{
	if (SkillId.IsNone() || Amount <= 0.f)
	{
		return;
	}

	const FSkillDefinitionRow* Def = GetSkillDef(SkillId);
	if (!Def)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skill] Missing SkillDef for %s"), *SkillId.ToString());
		return;
	}

	EnsureSkillExists(SkillId);

	FPageSkillRuntime& State = Skills.FindChecked(SkillId);
	State.TotalXP += Amount;
	State.Level = EvaluateLevelFromXP(*Def, State.TotalXP);

	if (bAllowPropagation)
	{
		for (const TPair<FName, float>& Pair : Def->RelatedSkillCoefficients)
		{
			const FName RelatedSkillId = Pair.Key;
			const float Coeff = Pair.Value;
			const float RelatedXP = Amount * Coeff;

			if (RelatedXP > 0.f)
			{
				AddSkillXP_Internal(RelatedSkillId, RelatedXP, false);
			}
		}
	}
}

int32 USkillComponent::GetSkillLevel(FName SkillId) const
{
	if (const FPageSkillRuntime* Found = Skills.Find(SkillId))
	{
		return Found->Level;
	}
	return 0;
}

float USkillComponent::GetSkillXP(FName SkillId) const
{
	if (const FPageSkillRuntime* Found = Skills.Find(SkillId))
	{
		return Found->TotalXP;
	}
	return 0.f;
}

float USkillComponent::GetSkillMultiplier(FName SkillId) const
{
	const FSkillDefinitionRow* Def = GetSkillDef(SkillId);
	if (!Def)
	{
		return 1.f;
	}

	const int32 Level = GetSkillLevel(SkillId);
	return 1.f + (float)Level * Def->MultiplierPerLevel;
}

