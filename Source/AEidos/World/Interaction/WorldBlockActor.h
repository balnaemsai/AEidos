#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/Interaction/WorldInteractionInterface.h"
#include "WorldBlockActor.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/** One action a world block may offer. Geometry is presentation only; all world terrain and objects use this model. */
USTRUCT(BlueprintType)
struct FWorldBlockInteractionDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block") FName InteractionId = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block") FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block", meta=(MultiLine="true")) FText Description;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block") FName RequiredToolTag = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block") FName ResultItemId = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block", meta=(ClampMin="1")) int32 ResultQuantity = 1;
	/** Amount of block integrity removed after a successful use of this action. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block", meta=(ClampMin="1", EditCondition="bConsumesIntegrity")) int32 IntegrityDamage = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block") bool bIsDefault = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block") bool bConsumesIntegrity = true;
};

/**
 * Common representation for every interactive world object: terrain, props,
 * containers, dropped objects, and harvestable material all behave as blocks.
 */
UCLASS(Blueprintable)
class AEIDOS_API AWorldBlockActor : public AActor, public IWorldInteractionInterface
{
	GENERATED_BODY()

public:
	AWorldBlockActor();

	virtual void GetAvailableWorldInteractions_Implementation(APageCharacter* InteractingPage,
		TArray<FWorldInteractionOption>& OutOptions) override;
	virtual bool ExecuteWorldInteraction_Implementation(APageCharacter* InteractingPage, FName InteractionId) override;

	UFUNCTION(BlueprintPure, Category="Block") virtual FName GetBlockId() const { return BlockId; }
	UFUNCTION(BlueprintPure, Category="Block") virtual FText GetBlockDisplayName() const;
	UFUNCTION(BlueprintPure, Category="Block") virtual int32 GetRemainingIntegrity() const { return RemainingIntegrity; }
	virtual void GetBlockInteractionDefinitions(TArray<FWorldBlockInteractionDefinition>& OutDefinitions) const;
	/** Refreshes this block from the CSV-backed DT_Block and DT_BlockInteraction tables. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Block") void ApplyBlockDefinition();
	UFUNCTION(BlueprintCallable, Category="Block") void SetInteractionFocused(bool bFocused);
	UFUNCTION(BlueprintPure, Category="Block") FVector GetInteractionFocusLocation() const;
	/** Used by the controller-only placement ghost. A real placed block is never left in this mode. */
	UFUNCTION(BlueprintCallable, Category="Block|Placement") void SetPlacementPreview(bool bInPreviewMode, bool bInPlacementValid = true);
	UFUNCTION(BlueprintPure, Category="Block|Placement") FVector GetPlacementBoundsExtent() const;

	/** Applies a dungeon-preset snapshot after this block is spawned into a runtime dungeon. */
	virtual void ApplyDungeonBlockPresetData(FName InBlockId, int32 InRemainingIntegrity,
		const TArray<FWorldBlockInteractionDefinition>& InInteractions);

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	bool CanInteract(APageCharacter* InteractingPage) const;
	bool HasRequiredTool(APageCharacter* InteractingPage, FName RequiredToolTag) const;
	bool GrantInteractionResult(APageCharacter* InteractingPage, const FWorldBlockInteractionDefinition& Interaction);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Block") TObjectPtr<UStaticMeshComponent> Mesh;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Block|Placement") TObjectPtr<UMaterialInterface> PlacementPreviewOverlayMaterial;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> PlacementPreviewOverlayMID;
	UPROPERTY(Transient) bool bPlacementPreviewMode = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Block") FName BlockId = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Block") FText BlockDisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Block", meta=(ClampMin="1")) int32 RemainingIntegrity = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Block") TArray<FWorldBlockInteractionDefinition> Interactions;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Block") bool bDestroyWhenDepleted = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Block", meta=(ClampMin="1.0")) float InteractionRangeCm = 250.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Block", meta=(ClampMin="0.0")) float FocusLabelOffsetCm = 18.f;
};
