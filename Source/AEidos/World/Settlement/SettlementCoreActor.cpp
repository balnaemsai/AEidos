#include "World/Settlement/SettlementCoreActor.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

ASettlementCoreActor::ASettlementCoreActor()
{
	PrimaryActorTick.bCanEverTick = false;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	CoreMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoreMesh"));
	CoreMesh->SetupAttachment(Root);
	CoreMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CoreMesh->SetCollisionObjectType(ECC_WorldDynamic);
	CoreMesh->SetCollisionResponseToAllChannels(ECR_Block);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		CoreMesh->SetStaticMesh(SphereMesh.Object);
		CoreMesh->SetRelativeScale3D(FVector(2.f));
	}

	CoreLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("CoreLight"));
	CoreLight->SetupAttachment(CoreMesh);
	CoreLight->SetIntensity(4000.f);
	CoreLight->SetLightColor(FLinearColor(0.75f, 0.82f, 1.f));
	CoreLight->SetAttenuationRadius(800.f);
}

void ASettlementCoreActor::ApplyCoreDamage(float DamageAmount)
{
	if (bDestroyed || DamageAmount <= 0.f)
	{
		return;
	}

	Health = FMath::Max(0.f, Health - DamageAmount);
	if (Health <= 0.f)
	{
		bDestroyed = true;
		OnCoreDestroyed.Broadcast(this);
		UE_LOG(LogTemp, Error, TEXT("[SettlementCore] Destroyed"));
	}
}

void ASettlementCoreActor::RestoreCoreState(float InHealth, float InMaxHealth, bool bInDestroyed)
{
	MaxHealth = FMath::Max(1.f, InMaxHealth);
	Health = FMath::Clamp(InHealth, 0.f, MaxHealth);
	bDestroyed = bInDestroyed || Health <= 0.f;
	SetActorHiddenInGame(bDestroyed);
	SetActorEnableCollision(!bDestroyed);
	if (CoreLight)
	{
		CoreLight->SetVisibility(!bDestroyed);
	}
}
