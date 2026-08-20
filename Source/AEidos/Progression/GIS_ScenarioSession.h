#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GIS_ScenarioSession.generated.h"

/** Carries a menu selection across the one map transition that begins a new game. */
UCLASS()
class AEIDOS_API UGIS_ScenarioSession : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Scenario")
	void SetPendingNewGameScenario(FName ScenarioId);

	void ClearPendingNewGameScenario();
	bool ConsumePendingNewGameScenario(FName& OutScenarioId);

private:
	FName PendingNewGameScenarioId = NAME_None;
};
