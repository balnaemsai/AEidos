// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PortalActor.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class UWS_PortalDirector;

UCLASS()
class AEIDOS_API APortalActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APortalActor();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable)
	void InitializePortal(int32 InPortalId, int32 InTier);

	UFUNCTION(BlueprintCallable)
	void Interact(APlayerController* InteractingPC);

	UFUNCTION(BlueprintPure)
	int32 GetPortalId() const { return PortalId; }

	UFUNCTION(BlueprintPure)
	int32 GetTier() const { return Tier; }

	UFUNCTION(BlueprintImplementableEvent)
	void BP_OnPortalInitialized();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* PortalMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USphereComponent* InteractionSphere;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Portal")
	float InteractionRadius = 200.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Portal")
	int32 PortalId = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Portal")
	int32 Tier = 1;

};
