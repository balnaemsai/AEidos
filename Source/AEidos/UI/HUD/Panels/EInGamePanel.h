#pragma once
#include "EInGamePanel.generated.h"

UENUM(BlueprintType)
enum class EInGamePanel : uint8
{
	None     UMETA(DisplayName="None"),
	Recruit  UMETA(DisplayName="Recruit"),
	Craft    UMETA(DisplayName="Craft"),
	Research UMETA(DisplayName="Research"),
	Build    UMETA(DisplayName="Build"),
	Buildings UMETA(DisplayName="Buildings"),
	Dungeons UMETA(DisplayName="Dungeons"),
	Pages    UMETA(DisplayName="Pages"),
	Items    UMETA(DisplayName="Items"),
	Relations UMETA(DisplayName="Relations"),
	Skill    UMETA(DisplayName="Skill"),
};
