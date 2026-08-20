#include "World/Settlement/WS_SettlementValue.h"

#include "Entities/Page/PageCharacter.h"
#include "Entities/Page/Components/SkillComponent.h"
#include "Data/GIS_DataRegistry.h"
#include "Engine/GameInstance.h"
#include "World/Settlement/WS_Building.h"
#include "World/Settlement/WS_Economy.h"
#include "World/Settlement/WS_ItemStorage.h"
#include "World/Settlement/WS_Population.h"
#include "World/Settlement/WS_SettlementSpace.h"

float UWS_SettlementValue::GetWarehouseResourceValue() const
{
	const UWS_Economy* Economy = GetWorld() ? GetWorld()->GetSubsystem<UWS_Economy>() : nullptr;
	if (!Economy)
	{
		return 0.f;
	}

	// Resource prices are intentionally not fixed yet. This shared baseline
	// includes every DataTable resource, so new resource rows need no code edit.
	float Total = 0.f;
	const UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const UGIS_DataRegistry* Registry = GameInstance ? GameInstance->GetSubsystem<UGIS_DataRegistry>() : nullptr;
	if (!Registry)
	{
		return Total;
	}

	for (const FName ResourceId : Registry->GetAllResourceIds())
	{
		Total += FMath::Max(0, Economy->GetAmount(ResourceId));
	}
	return Total;
}

float UWS_SettlementValue::GetWarehouseItemValue() const
{
	const UWS_ItemStorage* Storage = GetWorld() ? GetWorld()->GetSubsystem<UWS_ItemStorage>() : nullptr;
	if (!Storage)
	{
		return 0.f;
	}

	float Total = 0.f;
	for (const FItemStack& Stack : Storage->GetStoredItems())
	{
		Total += FMath::Max(0, Stack.Quantity) * 5.f;
	}
	return Total;
}

float UWS_SettlementValue::GetPageValue(const APageCharacter* Page) const
{
	if (!Page || !Page->IsFriendly())
	{
		return 0.f;
	}

	float Value = 100.f;
	if (const USkillComponent* Skills = Page->Skills)
	{
		for (const TPair<FName, FPageSkillRuntime>& Pair : Skills->GetAllSkillStates())
		{
			Value += FMath::Max(0, Pair.Value.Level) * 25.f;
		}
	}
	return Value;
}

float UWS_SettlementValue::GetCurrentSettlementValue() const
{
	float Value = GetWarehouseResourceValue() + GetWarehouseItemValue();

	if (const UWS_SettlementSpace* Space = GetWorld() ? GetWorld()->GetSubsystem<UWS_SettlementSpace>() : nullptr)
	{
		Value += Space->GetOwnedChunks().Num() * 100.f;
	}
	if (const UWS_Building* Buildings = GetWorld() ? GetWorld()->GetSubsystem<UWS_Building>() : nullptr)
	{
		TArray<FName> Completed;
		Buildings->GetCompletedBuildingIds(Completed);
		Value += Completed.Num() * 250.f;
	}
	if (const UWS_Population* Population = GetWorld() ? GetWorld()->GetSubsystem<UWS_Population>() : nullptr)
	{
		for (const TWeakObjectPtr<APageCharacter>& Page : Population->GetOwnedPages())
		{
			Value += GetPageValue(Page.Get());
		}
	}
	return FMath::Max(0.f, Value);
}

float UWS_SettlementValue::GetDifficultyMean() const
{
	return FMath::Max(0.5f, GetCurrentSettlementValue() / 500.f);
}

float UWS_SettlementValue::RollDungeonDifficulty(int32 Seed) const
{
	FRandomStream Stream(Seed);
	const float Mean = GetDifficultyMean();
	const float U1 = FMath::Max(Stream.FRand(), KINDA_SMALL_NUMBER);
	const float U2 = Stream.FRand();
	const float StandardNormal = FMath::Sqrt(-2.f * FMath::Loge(U1)) * FMath::Cos(2.f * PI * U2);
	return FMath::Clamp(Mean + StandardNormal * (Mean * 0.20f), 0.5f, 10.f);
}
