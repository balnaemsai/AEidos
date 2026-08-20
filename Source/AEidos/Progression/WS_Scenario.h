#pragma once

#include "CoreMinimal.h"
#include "Save/SaveGameParticipant.h"
#include "Simulation/SimSystem.h"
#include "Subsystems/WorldSubsystem.h"
#include "WS_Scenario.generated.h"

class UWS_DungeonRuntime;
struct FScenarioDefinitionRow;
class USimCommandBuffer;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnScenarioStateChanged);

/** Durable, data-driven scenario evaluator. A scenario is inactive until selected. */
UCLASS()
class AEIDOS_API UWS_Scenario : public UWorldSubsystem, public ISimSystem, public ISaveGameParticipant
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void SimPlan_Implementation(USimCommandBuffer* CommandBuffer, float FixedDeltaSeconds) override;
	virtual int32 GetSimOrder_Implementation() const override { return 120; }
	virtual void WriteToSnapshot_Implementation(FEidosWorldSnapshot& InOutSnapshot) const override;
	virtual void ApplySnapshot_Implementation(const FEidosWorldSnapshot& Snapshot) override;

	UFUNCTION(BlueprintCallable, Category="Scenario")
	bool SelectScenario(FName ScenarioId);

	UFUNCTION(BlueprintPure, Category="Scenario")
	FName GetActiveScenarioId() const { return ActiveScenarioId; }

	UFUNCTION(BlueprintPure, Category="Scenario")
	bool IsScenarioCompleted() const { return bCompleted; }

	UFUNCTION(BlueprintPure, Category="Scenario")
	int32 GetDestroyedDungeonCoreCount() const { return DestroyedDungeonCoreCount; }

	UPROPERTY(BlueprintAssignable, Category="Scenario")
	FOnScenarioStateChanged OnScenarioStateChanged;

private:
	const FScenarioDefinitionRow* FindActiveDefinition() const;
	bool AreRequirementsMet(const FScenarioDefinitionRow& Definition) const;
	void HandleDungeonCoreDestroyed(int32 PortalId);

	FName ActiveScenarioId = NAME_None;
	int32 DestroyedDungeonCoreCount = 0;
	bool bCompleted = false;
	TWeakObjectPtr<UWS_DungeonRuntime> DungeonRuntime;
	FDelegateHandle DungeonCoreDestroyedHandle;
};
