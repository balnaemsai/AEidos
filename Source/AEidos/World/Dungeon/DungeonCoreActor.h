#pragma once

#include "CoreMinimal.h"
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
	FName CoreShardItemId = TEXT("CoreShard_Wood");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon|Core|Rewards", meta=(ClampMin="1"))
	int32 CoreShardQuantity = 1;

	UPROPERTY(Transient)
	bool bCoreDestroyed = false;

	TWeakObjectPtr<APageCharacter> LastDamageInstigator;
};
