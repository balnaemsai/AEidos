#pragma once

#include "CoreMinimal.h"
#include "Core/Types/ItemTypes.h"
#include "GameFramework/Actor.h"
#include "DungeonCoreActor.generated.h"

class UPointLightComponent;
class UStaticMeshComponent;
class APageCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDungeonCoreDestroyedSignature, ADungeonCoreActor*, DungeonCore);

UCLASS()
class AEIDOS_API ADungeonCoreActor : public AActor
{
	GENERATED_BODY()

public:
	ADungeonCoreActor();

	UFUNCTION(BlueprintCallable, Category="Dungeon|Core")
	void ApplyCoreDamage(float DamageAmount, APageCharacter* DamageInstigator = nullptr);

	UFUNCTION(BlueprintCallable, Category="Dungeon|Core")
	void Interact(APlayerController* InteractingController);

	UFUNCTION(BlueprintPure, Category="Dungeon|Core")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category="Dungeon|Core")
	float GetMaxHealth() const { return MaxHealth; }

	/** Reward metadata is consumed by the dungeon runtime to spawn a recoverable world item. */
	UFUNCTION(BlueprintPure, Category="Dungeon|Core|Rewards")
	FName GetCoreShardItemId() const { return CoreShardItemId; }

	UFUNCTION(BlueprintPure, Category="Dungeon|Core|Rewards")
	int32 GetCoreShardQuantity() const { return CoreShardQuantity; }

	const TArray<FItemStack>& GetCoreShardRewards() const { return CoreShardRewards; }
	void ConfigureCoreShardRewards(const TArray<FItemStack>& InRewards);

	UPROPERTY(BlueprintAssignable, Category="Dungeon|Core")
	FOnDungeonCoreDestroyedSignature OnCoreDestroyed;

protected:
	void DestroyCore();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dungeon|Core")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dungeon|Core")
	TObjectPtr<UStaticMeshComponent> CoreMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dungeon|Core")
	TObjectPtr<UPointLightComponent> CoreLight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon|Core", meta=(ClampMin="1.0"))
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon|Core", meta=(ClampMin="0.0"))
	float Health = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon|Core", meta=(ClampMin="50.0"))
	float InteractDistanceCm = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon|Core|Rewards")
	FName CoreShardItemId = TEXT("PortalShard");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon|Core|Rewards", meta=(ClampMin="1"))
	int32 CoreShardQuantity = 1;

	/** Runtime data wins when present; the legacy single item remains a Blueprint fallback. */
	UPROPERTY(Transient)
	TArray<FItemStack> CoreShardRewards;

	UPROPERTY(Transient)
	bool bCoreDestroyed = false;
};
