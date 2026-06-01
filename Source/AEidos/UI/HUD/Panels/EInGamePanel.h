#pragma once
#include "EInGamePanel.generated.h"

UENUM(BlueprintType)
enum class EInGamePanel : uint8
{
	None      UMETA(DisplayName="None"),
	Buildings UMETA(DisplayName="Buildings"),
	Pages     UMETA(DisplayName="Pages"),
	Dungeons  UMETA(DisplayName="Dungeons"),
	Items     UMETA(DisplayName="Items"),
	Research  UMETA(DisplayName="Research"),
};
