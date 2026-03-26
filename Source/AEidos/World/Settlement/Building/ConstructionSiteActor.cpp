#include "World/Settlement/Building/ConstructionSiteActor.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

AConstructionSiteActor::AConstructionSiteActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	SiteMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SiteMesh"));
	SiteMesh->SetupAttachment(Root);
	SiteMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	FootprintCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("FootprintCollision"));
	FootprintCollision->SetupAttachment(Root);
	FootprintCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	FootprintCollision->SetCollisionObjectType(ECC_WorldDynamic);
	FootprintCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	FootprintCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	RefreshCollisionExtent();
}

void AConstructionSiteActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	RefreshCollisionExtent();
	RefreshPreviewVisual();
}

void AConstructionSiteActor::InitializeConstructionSite(int32 InSiteId, FName InBuildingId, int32 InWorkRequestId)
{
	SiteId = InSiteId;
	BuildingId = InBuildingId;
	WorkRequestId = InWorkRequestId;
}

void AConstructionSiteActor::SetFootprint(const FVector2D& InFootprint)
{
	Footprint.X = FMath::Max(1.f, InFootprint.X);
	Footprint.Y = FMath::Max(1.f, InFootprint.Y);

	RefreshCollisionExtent();
}

void AConstructionSiteActor::SetPreviewValid(bool bIsValidPlacement)
{
	bPreviewValid = bIsValidPlacement;
	RefreshPreviewVisual();
}

void AConstructionSiteActor::SetConstructionProgress(float InNormalizedProgress)
{
	ConstructionProgress = FMath::Clamp(InNormalizedProgress, 0.f, 1.f);

	if (!PreviewMID && SiteMesh)
	{
		PreviewMID = SiteMesh->CreateAndSetMaterialInstanceDynamic(0);
	}

	if (PreviewMID)
	{
		// 머티리얼에 Progress 스칼라 파라미터가 있으면 반영
		PreviewMID->SetScalarParameterValue(TEXT("Progress"), ConstructionProgress);
	}
}

void AConstructionSiteActor::RefreshCollisionExtent()
{
	if (!FootprintCollision)
	{
		return;
	}

	const FVector Extent(Footprint.X * 0.5f, Footprint.Y * 0.5f, 80.f);
	FootprintCollision->SetBoxExtent(Extent);
}

void AConstructionSiteActor::RefreshPreviewVisual()
{
	if (!SiteMesh)
	{
		return;
	}

	if (!PreviewMID)
	{
		PreviewMID = SiteMesh->CreateAndSetMaterialInstanceDynamic(0);
	}

	if (!PreviewMID)
	{
		return;
	}

	// 머티리얼에 TintColor / Opacity 파라미터가 있다고 가정
	const FLinearColor ValidColor   = FLinearColor(0.15f, 1.0f, 0.2f, 1.f);
	const FLinearColor InvalidColor = FLinearColor(1.0f, 0.15f, 0.15f, 1.f);

	PreviewMID->SetVectorParameterValue(TEXT("TintColor"), bPreviewValid ? ValidColor : InvalidColor);
	PreviewMID->SetScalarParameterValue(TEXT("Opacity"), 0.6f);
}