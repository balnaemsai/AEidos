#pragma once

#include "CoreMinimal.h"
#include "Core/Types/ItemTypes.h"
#include "Save/SaveGameParticipant.h"
#include "Subsystems/WorldSubsystem.h"
#include "WS_ItemStorage.generated.h"

class APageCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSettlementStorageChanged);

/**
 * Shared, immediately usable warehouse storage. Physical world stockpiles are
 * intentionally outside this subsystem and therefore cannot pay work costs.
 */
UCLASS()
class AEIDOS_API UWS_ItemStorage : public UWorldSubsystem, public ISaveGameParticipant
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void WriteToSnapshot_Implementation(FEidosWorldSnapshot& OutSnapshot) const override;
	virtual void ApplySnapshot_Implementation(const FEidosWorldSnapshot& Snapshot) override;

	UFUNCTION(BlueprintPure, Category="Storage")
	float GetTotalWeightCapacity() const;

	UFUNCTION(BlueprintPure, Category="Storage")
	float GetTotalVolumeCapacity() const;

	UFUNCTION(BlueprintPure, Category="Storage")
	float GetCurrentWeight() const;

	UFUNCTION(BlueprintPure, Category="Storage")
	float GetCurrentVolume() const;

	UFUNCTION(BlueprintPure, Category="Storage")
	const TArray<FItemStack>& GetStoredItems() const { return StoredItems; }

	UFUNCTION(BlueprintPure, Category="Storage")
	int32 GetStoredItemAmount(FName ItemId) const;

	/** Checks aggregate weight and volume before a work order deposits several item rewards. */
	bool CanStoreItemStacks(const TArray<FItemStack>& ItemStacks) const;

	int32 GetMaxResourceAmountThatFits(FName ResourceId, int32 RequestedAmount) const;

	UFUNCTION(BlueprintCallable, Category="Storage")
	int32 TryStoreItem(FName ItemId, int32 RequestedQuantity, float TotalQuality = 0.f);

	UFUNCTION(BlueprintCallable, Category="Storage")
	int32 TryTakeStoredItem(FName ItemId, int32 RequestedQuantity, float& OutRemovedQuality);

	UFUNCTION(BlueprintCallable, Category="Storage")
	void DepositPageInventory(APageCharacter* Page);

	/** Converts only return-convertible resource items carried by a Page. Equipment and ordinary items stay carried. */
	UFUNCTION(BlueprintCallable, Category="Storage")
	void ConvertReturnResources(APageCharacter* Page);

	void NotifyResourceChanged();

	UPROPERTY(BlueprintAssignable, Category="Storage")
	FOnSettlementStorageChanged OnStorageChanged;

protected:
	UPROPERTY(EditDefaultsOnly, Category="Storage", meta=(ClampMin="0.0"))
	float BaseWeightCapacity = 1000.f;

	UPROPERTY(EditDefaultsOnly, Category="Storage", meta=(ClampMin="0.0"))
	float BaseVolumeCapacity = 2000.f;

	UPROPERTY(VisibleAnywhere, Category="Storage")
	TArray<FItemStack> StoredItems;

private:
	const struct FItemDefinitionRow* FindItemDefinition(FName ItemId) const;
	const struct FResourceDefinitionRow* FindResourceDefinition(FName ResourceId) const;
	void BroadcastChanged();
	static FName SnapshotKey();
	static FString EncodeStacks(const TArray<FItemStack>& Stacks);
	static void DecodeStacks(const FString& Encoded, TArray<FItemStack>& OutStacks);
};
