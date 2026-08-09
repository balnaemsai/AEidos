#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DungeonSettlementAuthoringActor.generated.h"

class UDungeonSettlementPreset;
class USceneComponent;

UCLASS()
class AEIDOS_API ADungeonSettlementAuthoringActor : public AActor
{
	GENERATED_BODY()

public:
	ADungeonSettlementAuthoringActor();

#if WITH_EDITOR
	UFUNCTION(CallInEditor, Category="Dungeon|Preset")
	void CapturePresetFromLevel();
#endif

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dungeon")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon")
	TObjectPtr<UDungeonSettlementPreset> TargetPreset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon")
	float ChunkSizeCm = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon")
	bool bCaptureTerritoryChunks = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon")
	bool bCaptureBuildings = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon")
	bool bCaptureMarkers = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon")
	bool bCaptureWorldBlocks = true;

	/** Legacy toggle retained for existing authoring actors. New captures use bCaptureWorldBlocks. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon", meta=(DeprecatedProperty, DeprecationMessage="Use Capture World Blocks."))
	bool bCaptureWorldItemBlocks = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon")
	bool bRestrictCaptureToSameLevel = false;

private:
#if WITH_EDITOR
	FTransform MakeLocalTransform(const FTransform& WorldTransform) const;
#endif
};
