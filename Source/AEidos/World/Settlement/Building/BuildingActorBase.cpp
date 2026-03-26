#include "World/Settlement/Building/BuildingActorBase.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"

ABuildingActorBase::ABuildingActorBase()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	BuildingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BuildingMesh"));
	BuildingMesh->SetupAttachment(Root);
	BuildingMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BuildingMesh->SetCollisionObjectType(ECC_WorldStatic);
	BuildingMesh->SetCollisionResponseToAllChannels(ECR_Block);

	FootprintCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("FootprintCollision"));
	FootprintCollision->SetupAttachment(Root);
	FootprintCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	FootprintCollision->SetCollisionObjectType(ECC_WorldStatic);
	FootprintCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	FootprintCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	RefreshCollisionExtent();
}

void ABuildingActorBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshCollisionExtent();
}

void ABuildingActorBase::InitializeBuilding(FName InBuildingId, int32 InUpgradeLevel)
{
	BuildingId = InBuildingId;
	UpgradeLevel = FMath::Max(1, InUpgradeLevel);
}

void ABuildingActorBase::SetFootprint(const FVector2D& InFootprint)
{
	Footprint.X = FMath::Max(1.f, InFootprint.X);
	Footprint.Y = FMath::Max(1.f, InFootprint.Y);

	RefreshCollisionExtent();
}

void ABuildingActorBase::SetBuildingActive(bool bInActive)
{
	bBuildingActive = bInActive;

	SetActorHiddenInGame(!bBuildingActive);
	SetActorEnableCollision(bBuildingActive);
	SetActorTickEnabled(false);
}

void ABuildingActorBase::RefreshCollisionExtent()
{
	if (!FootprintCollision)
	{
		return;
	}

	// Z는 적당한 기본 높이. 나중에 DT/메시 기준으로 늘려도 됨.
	const FVector Extent(Footprint.X * 0.5f, Footprint.Y * 0.5f, 100.f);
	FootprintCollision->SetBoxExtent(Extent);
}