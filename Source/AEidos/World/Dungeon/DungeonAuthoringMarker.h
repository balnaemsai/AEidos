#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DungeonAuthoringMarker.generated.h"

class UArrowComponent;
class USceneComponent;

UENUM(BlueprintType)
enum class EDungeonAuthoringMarkerType : uint8
{
	Entry,
	Core,
	EnemySpawn
};

UCLASS()
class AEIDOS_API ADungeonAuthoringMarker : public AActor
{
	GENERATED_BODY()

public:
	ADungeonAuthoringMarker();

	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon")
	EDungeonAuthoringMarkerType MarkerType = EDungeonAuthoringMarkerType::Entry;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon", meta=(ClampMin="1"))
	int32 EnemySpawnCount = 1;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dungeon")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dungeon")
	TObjectPtr<UArrowComponent> Arrow;
};
