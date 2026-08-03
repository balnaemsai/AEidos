#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DungeonReturnPortalActor.generated.h"

class UPointLightComponent;
class USphereComponent;
class UStaticMeshComponent;

/**
 * Runtime-only portal created where a dungeon core collapses. Each friendly
 * Page must interact with it to return before the dungeon collapse timer ends.
 */
UCLASS()
class AEIDOS_API ADungeonReturnPortalActor : public AActor
{
	GENERATED_BODY()

public:
	ADungeonReturnPortalActor();

	UFUNCTION(BlueprintCallable, Category="Dungeon|Return Portal")
	void Interact(APlayerController* InteractingController);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dungeon|Return Portal")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dungeon|Return Portal")
	TObjectPtr<UStaticMeshComponent> PortalMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dungeon|Return Portal")
	TObjectPtr<USphereComponent> InteractionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dungeon|Return Portal")
	TObjectPtr<UPointLightComponent> PortalLight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon|Return Portal", meta=(ClampMin="50.0"))
	float InteractionRadius = 250.f;
};
