#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/Types/ItemTypes.h"
#include "EquipmentComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEquipmentChanged);

class UInventoryComponent;

UCLASS(ClassGroup=(Eidos), meta=(BlueprintSpawnableComponent))
class AEIDOS_API UEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEquipmentComponent();
	UFUNCTION(BlueprintPure, Category="Equipment") const TArray<FPageEquipmentSlotState>& GetEquippedSlots() const { return EquippedSlots; }
	UFUNCTION(BlueprintCallable, Category="Equipment") void SetEquippedSlots(const TArray<FPageEquipmentSlotState>& InSlots);
	UFUNCTION(BlueprintPure, Category="Equipment") FName GetEquippedItem(EPageEquipmentSlot Slot) const;
	UFUNCTION(BlueprintPure, Category="Equipment") FName GetActiveToolItem() const;
	UFUNCTION(BlueprintPure, Category="Equipment") bool HasActiveToolTag(FName ToolTag) const;
	UFUNCTION(BlueprintCallable, Category="Equipment") bool EquipFromInventory(FName ItemId, EPageEquipmentSlot Slot);
	UFUNCTION(BlueprintCallable, Category="Equipment") bool UnequipToInventory(EPageEquipmentSlot Slot);

	UPROPERTY(BlueprintAssignable, Category="Equipment") FOnEquipmentChanged OnEquipmentChanged;

private:
	const struct FItemDefinitionRow* FindItemDefinition(FName ItemId) const;
	UInventoryComponent* GetOwnerInventory() const;
	FPageEquipmentSlotState* FindSlot(EPageEquipmentSlot Slot);
	const FPageEquipmentSlotState* FindSlot(EPageEquipmentSlot Slot) const;
	void EnsureSlots();

	UPROPERTY(VisibleAnywhere, Category="Equipment") TArray<FPageEquipmentSlotState> EquippedSlots;
};
