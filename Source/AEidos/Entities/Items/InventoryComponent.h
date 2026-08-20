#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/Types/ItemTypes.h"
#include "InventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);

UCLASS(ClassGroup=(Eidos), meta=(BlueprintSpawnableComponent))
class AEIDOS_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	UFUNCTION(BlueprintPure, Category="Inventory")
	const TArray<FItemStack>& GetStacks() const { return Stacks; }

	UFUNCTION(BlueprintPure, Category="Inventory")
	float GetCurrentWeight() const;

	UFUNCTION(BlueprintPure, Category="Inventory")
	float GetCurrentVolume() const;

	UFUNCTION(BlueprintPure, Category="Inventory")
	float GetMaxWeight() const { return MaxWeight; }

	UFUNCTION(BlueprintPure, Category="Inventory")
	float GetMaxVolume() const { return MaxVolume; }

	UFUNCTION(BlueprintCallable, Category="Inventory")
	void SetCapacity(float InMaxWeight, float InMaxVolume);

	UFUNCTION(BlueprintCallable, Category="Inventory")
	int32 TryAddItem(FName ItemId, int32 RequestedQuantity, float TotalQuality = 0.f);

	/** Adds a full runtime stack without discarding instance data such as portal-shard attributes. */
	int32 TryAddItemStack(const FItemStack& ItemStack);

	UFUNCTION(BlueprintCallable, Category="Inventory")
	int32 TryRemoveItem(FName ItemId, int32 RequestedQuantity, float& OutRemovedQuality);

	/** Removes an identical instance stack. Intended for non-stackable, stateful items. */
	bool TryRemoveItemStack(const FItemStack& ItemStack);

	UFUNCTION(BlueprintCallable, Category="Inventory")
	void SetStacks(const TArray<FItemStack>& InStacks);

	UPROPERTY(BlueprintAssignable, Category="Inventory")
	FOnInventoryChanged OnInventoryChanged;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inventory", meta=(ClampMin="0.0"))
	float MaxWeight = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inventory", meta=(ClampMin="0.0"))
	float MaxVolume = 500.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory")
	TArray<FItemStack> Stacks;

private:
	const struct FItemDefinitionRow* FindDefinition(FName ItemId) const;
	void BroadcastChanged();
};
