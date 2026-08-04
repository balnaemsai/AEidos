#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/Interaction/WorldInteractionInterface.h"
#include "WorldItemBlockActor.generated.h"

class UStaticMeshComponent;

UCLASS()
class AEIDOS_API AWorldItemBlockActor : public AActor, public IWorldInteractionInterface
{
	GENERATED_BODY()

public:
	AWorldItemBlockActor();
	virtual void GetAvailableWorldInteractions_Implementation(APageCharacter* InteractingPage, TArray<FWorldInteractionOption>& OutOptions) override;
	virtual bool ExecuteWorldInteraction_Implementation(APageCharacter* InteractingPage, FName InteractionId) override;
	UFUNCTION(BlueprintPure, Category="World Item") int32 GetRemainingQuantity() const { return RemainingQuantity; }

protected:
	bool CanInteract(APageCharacter* InteractingPage) const;
	bool HasRequiredTool(APageCharacter* InteractingPage) const;
	bool TransferToInventory(APageCharacter* InteractingPage, int32 Amount);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="World Item") TObjectPtr<UStaticMeshComponent> Mesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="World Item") FName ItemId = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="World Item", meta=(ClampMin="1")) int32 RemainingQuantity = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="World Item", meta=(ClampMin="1")) int32 QuantityPerHarvest = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="World Item") bool bCanPickUp = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="World Item") bool bCanHarvest = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="World Item", meta=(EditCondition="bCanHarvest")) FName RequiredHarvestToolTag = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="World Item", meta=(ClampMin="1.0")) float InteractionRangeCm = 250.f;
};
