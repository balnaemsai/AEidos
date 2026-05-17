#include "World/Dungeon/DungeonAuthoringMarker.h"

#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"

ADungeonAuthoringMarker::ADungeonAuthoringMarker()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Arrow = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	Arrow->SetupAttachment(Root);
	Arrow->ArrowSize = 2.0f;
	Arrow->SetHiddenInGame(false);
}

void ADungeonAuthoringMarker::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	FColor MarkerColor = FColor::Cyan;
	switch (MarkerType)
	{
	case EDungeonAuthoringMarkerType::Entry:
		MarkerColor = FColor(80, 180, 255);
		break;
	case EDungeonAuthoringMarkerType::Core:
		MarkerColor = FColor(255, 220, 80);
		break;
	case EDungeonAuthoringMarkerType::EnemySpawn:
		MarkerColor = FColor(255, 90, 90);
		break;
	default:
		break;
	}

	if (Arrow)
	{
		Arrow->ArrowColor = MarkerColor;
	}
}
