#include "World/Dungeon/DungeonCoreActor.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Framework/EidosPlayerController.h"
#include "Entities/Page/PageCharacter.h"
#include "UObject/ConstructorHelpers.h"

ADungeonCoreActor::ADungeonCoreActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	CoreMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoreMesh"));
	CoreMesh->SetupAttachment(Root);
	// The core must be hittable by the gameplay camera trace as well as visible.
	CoreMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CoreMesh->SetCollisionObjectType(ECC_WorldDynamic);
	CoreMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	CoreMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	CoreMesh->SetRelativeScale3D(FVector(1.5f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		CoreMesh->SetStaticMesh(SphereMesh.Object);
	}

	CoreLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("CoreLight"));
	CoreLight->SetupAttachment(CoreMesh);
	CoreLight->SetIntensity(5000.f);
	CoreLight->SetLightColor(FLinearColor(1.0f, 0.2f, 0.2f).ToFColor(true));
	CoreLight->SetAttenuationRadius(600.f);
	CoreLight->SetRelativeLocation(FVector(0.f, 0.f, 40.f));
}

void ADungeonCoreActor::ConfigureCoreShardRewards(const TArray<FItemStack>& InRewards)
{
	CoreShardRewards.Reset();
	for (const FItemStack& Reward : InRewards)
	{
		if (Reward.IsValid())
		{
			CoreShardRewards.Add(Reward);
		}
	}
}

void ADungeonCoreActor::ApplyCoreDamage(float DamageAmount, APageCharacter* DamageInstigator)
{
	if (bCoreDestroyed || DamageAmount <= 0.f)
	{
		return;
	}

	Health = FMath::Max(0.f, Health - DamageAmount);
	if (Health <= 0.f)
	{
		DestroyCore();
	}
}

void ADungeonCoreActor::Interact(APlayerController* InteractingController)
{
	if (bCoreDestroyed || !InteractingController)
	{
		return;
	}

	AEidosPlayerController* EidosPC = Cast<AEidosPlayerController>(InteractingController);
	APageCharacter* SelectedPage = EidosPC ? EidosPC->GetSelectedPage() : nullptr;
	if (!SelectedPage || !SelectedPage->IsInDungeon())
	{
		return;
	}

	const float DistanceSq = FVector::DistSquared(SelectedPage->GetActorLocation(), GetActorLocation());
	if (DistanceSq > FMath::Square(InteractDistanceCm))
	{
		return;
	}

	// The core is a hostile objective. Interaction selects it; combat skills are the only way to destroy it.
	EidosPC->SelectCombatTarget(this);
}

void ADungeonCoreActor::DestroyCore()
{
	if (bCoreDestroyed)
	{
		return;
	}

	bCoreDestroyed = true;
	// The shard is not granted to the attacker. The dungeon runtime creates a
	// world item at this location so the expedition must recover it before collapse.
	OnCoreDestroyed.Broadcast(this);
	Destroy();
}
