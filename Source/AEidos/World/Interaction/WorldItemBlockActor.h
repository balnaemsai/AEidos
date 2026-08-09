#pragma once

#include "CoreMinimal.h"
#include "World/Interaction/WorldBlockActor.h"
#include "WorldItemBlockActor.generated.h"

UCLASS()
class AEIDOS_API AWorldItemBlockActor : public AWorldBlockActor
{
	GENERATED_BODY()

public:
	AWorldItemBlockActor();
	virtual void GetAvailableWorldInteractions_Implementation(APageCharacter* InteractingPage, TArray<FWorldInteractionOption>& OutOptions) override;
	virtual bool ExecuteWorldInteraction_Implementation(APageCharacter* InteractingPage, FName InteractionId) override;
	UFUNCTION(BlueprintPure, Category="World Item") int32 GetRemainingQuantity() const { return RemainingQuantity; }
	UFUNCTION(BlueprintPure, Category="World Item") FName GetItemId() const { return ItemId; }
	UFUNCTION(BlueprintPure, Category="World Item") int32 GetQuantityPerHarvest() const { return QuantityPerHarvest; }
	UFUNCTION(BlueprintPure, Category="World Item") bool CanPickUp() const { return bCanPickUp; }
	UFUNCTION(BlueprintPure, Category="World Item") bool CanHarvest() const { return bCanHarvest; }
	UFUNCTION(BlueprintPure, Category="World Item") FName GetRequiredHarvestToolTag() const { return RequiredHarvestToolTag; }
	virtual FName GetBlockId() const override { return ItemId; }
	virtual int32 GetRemainingIntegrity() const override { return RemainingQuantity; }
	virtual void GetBlockInteractionDefinitions(TArray<FWorldBlockInteractionDefinition>& OutDefinitions) const override;
	virtual void ApplyDungeonBlockPresetData(FName InBlockId, int32 InRemainingIntegrity,
		const TArray<FWorldBlockInteractionDefinition>& InInteractions) override;

	/** Applies an authoring-preset snapshot after the actor is spawned into a runtime dungeon. */
	void ApplyDungeonPresetData(FName InItemId, int32 InRemainingQuantity, int32 InQuantityPerHarvest,
		bool bInCanPickUp, bool bInCanHarvest, FName InRequiredHarvestToolTag);

	/** Initializes an item block spawned when a Page drops an inventory stack into the world. */
	UFUNCTION(BlueprintCallable, Category="World Item")
	void InitializeWorldItem(FName InItemId, int32 InQuantity);

protected:
	bool CanInteract(APageCharacter* InteractingPage) const;
	bool HasRequiredTool(APageCharacter* InteractingPage) const;
	bool TransferToInventory(APageCharacter* InteractingPage, int32 Amount);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="World Item") FName ItemId = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="World Item", meta=(ClampMin="1")) int32 RemainingQuantity = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="World Item", meta=(ClampMin="1")) int32 QuantityPerHarvest = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="World Item") bool bCanPickUp = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="World Item") bool bCanHarvest = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="World Item", meta=(EditCondition="bCanHarvest")) FName RequiredHarvestToolTag = NAME_None;
};
