#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SettlementCoreActor.generated.h"

class UPointLightComponent;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSettlementCoreDestroyed, ASettlementCoreActor*, SettlementCore);

/** The settlement's defeat objective. Raiders target this actor, never the dungeon core. */
UCLASS()
class AEIDOS_API ASettlementCoreActor : public AActor
{
	GENERATED_BODY()

public:
	ASettlementCoreActor();

	UFUNCTION(BlueprintCallable, Category="Settlement|Core")
	void ApplyCoreDamage(float DamageAmount);

	UFUNCTION(BlueprintPure, Category="Settlement|Core")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category="Settlement|Core")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category="Settlement|Core")
	bool IsDestroyed() const { return bDestroyed; }

	/** Applies persisted state before raids are allowed to target the core. */
	void RestoreCoreState(float InHealth, float InMaxHealth, bool bInDestroyed);

	UPROPERTY(BlueprintAssignable, Category="Settlement|Core")
	FOnSettlementCoreDestroyed OnCoreDestroyed;

private:
	UPROPERTY(VisibleAnywhere, Category="Settlement|Core")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category="Settlement|Core")
	TObjectPtr<UStaticMeshComponent> CoreMesh;

	UPROPERTY(VisibleAnywhere, Category="Settlement|Core")
	TObjectPtr<UPointLightComponent> CoreLight;

	UPROPERTY(EditAnywhere, Category="Settlement|Core", meta=(ClampMin="1.0"))
	float MaxHealth = 500.f;

	UPROPERTY(EditAnywhere, Category="Settlement|Core", meta=(ClampMin="0.0"))
	float Health = 500.f;

	bool bDestroyed = false;
};
