#pragma once

#include "CoreMinimal.h"
#include "Save/SaveGameParticipant.h"
#include "Subsystems/WorldSubsystem.h"
#include "WS_SettlementCore.generated.h"

class ASettlementCoreActor;

/**
 * Owns the settlement core lifecycle. The core is spawned only after the
 * settlement snapshot has restored its territory, never from a placed MenuMap actor.
 */
UCLASS()
class AEIDOS_API UWS_SettlementCore : public UWorldSubsystem, public ISaveGameParticipant
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	virtual void WriteToSnapshot_Implementation(FEidosWorldSnapshot& OutSnapshot) const override;
	virtual void ApplySnapshot_Implementation(const FEidosWorldSnapshot& Snapshot) override;

	/** Call after settlement-space snapshot application to create or restore the objective. */
	ASettlementCoreActor* EnsureSettlementCore();

	UFUNCTION(BlueprintPure, Category="Settlement|Core")
	ASettlementCoreActor* GetSettlementCore() const { return ActiveCore.Get(); }

private:
	FVector ResolveDefaultCoreLocation() const;
	ASettlementCoreActor* FindExistingCore() const;

	UPROPERTY(Transient)
	TWeakObjectPtr<ASettlementCoreActor> ActiveCore;

	UPROPERTY(EditDefaultsOnly, Category="Settlement|Core")
	TSubclassOf<ASettlementCoreActor> SettlementCoreClass;

	UPROPERTY(EditDefaultsOnly, Category="Settlement|Core")
	float CoreElevationCm = 220.f;

	bool bHasSavedLocation = false;
	FVector SavedLocation = FVector::ZeroVector;
	float SavedHealth = 500.f;
	float SavedMaxHealth = 500.f;
	bool bSavedDestroyed = false;

	static const FName KEY_Location;
	static const FName KEY_Health;
	static const FName KEY_MaxHealth;
	static const FName KEY_Destroyed;
};
