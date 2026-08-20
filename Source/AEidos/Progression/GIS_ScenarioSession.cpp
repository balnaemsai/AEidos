#include "Progression/GIS_ScenarioSession.h"

void UGIS_ScenarioSession::SetPendingNewGameScenario(FName ScenarioId)
{
	PendingNewGameScenarioId = ScenarioId;
}

void UGIS_ScenarioSession::ClearPendingNewGameScenario()
{
	PendingNewGameScenarioId = NAME_None;
}

bool UGIS_ScenarioSession::ConsumePendingNewGameScenario(FName& OutScenarioId)
{
	OutScenarioId = PendingNewGameScenarioId;
	PendingNewGameScenarioId = NAME_None;
	return !OutScenarioId.IsNone();
}
