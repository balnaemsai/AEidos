// Fill out your copyright notice in the Description page of Project Settings.


#include "World/Settlement/TerritoryChunkActor.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInstanceDynamic.h"

ATerritoryChunkActor::ATerritoryChunkActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);

	// 기본 Plane (100cm x 100cm)
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneFinder(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneFinder.Succeeded())
	{
		PlaneMesh = PlaneFinder.Object;
	}

	if (PlaneMesh)
	{
		Mesh->SetStaticMesh(PlaneMesh);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PreviewOverlayFinder(
		TEXT("/Engine/EngineDebugMaterials/M_SimpleTranslucent.M_SimpleTranslucent"));
	if (PreviewOverlayFinder.Succeeded())
	{
		PreviewOverlayMaterial = PreviewOverlayFinder.Object;
	}

	Mesh->SetMobility(EComponentMobility::Static);

	// 플레이어가 올라갈 수 있어야 함
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));
}

void ATerritoryChunkActor::InitChunk(const FIntPoint& InCoord, float InChunkSizeCm)
{
	Coord = InCoord;

	// Plane 기본 크기 100cm -> 1000cm(10m) 만들려면 Scale=10
	const float Scale = InChunkSizeCm / 100.f; // 100cm plane 기준
	Mesh->SetWorldScale3D(FVector(Scale, Scale, 1.f));

	UMaterialInterface* MaterialToUse = TerritoryMaterial;
	if (!MaterialToUse)
	{
		MaterialToUse = Mesh->GetMaterial(0);
	}

	if (MaterialToUse)
	{
		Mesh->SetMaterial(0, MaterialToUse);
		MID = Mesh->CreateDynamicMaterialInstance(0, MaterialToUse);
	}

	if (PreviewOverlayMaterial)
	{
		PreviewOverlayMID = UMaterialInstanceDynamic::Create(PreviewOverlayMaterial, this);
	}

	RefreshPreviewVisual();
}

#if WITH_EDITOR
void ATerritoryChunkActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (bPreviewMode || !bUseEditorAuthoringCoord)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (World->IsGameWorld())
		{
			return;
		}
	}

	if (!Mesh)
	{
		return;
	}

	Coord = EditorCoord;

	const float AppliedChunkSize = FMath::Max(100.f, EditorChunkSizeCm);
	const float Scale = AppliedChunkSize / 100.f;
	Mesh->SetWorldScale3D(FVector(Scale, Scale, 1.f));

	const FVector LocalLocation(EditorCoord.X * AppliedChunkSize, EditorCoord.Y * AppliedChunkSize, 0.f);
	SetActorLocation(LocalLocation);

	UMaterialInterface* MaterialToUse = TerritoryMaterial;
	if (!MaterialToUse)
	{
		MaterialToUse = Mesh->GetMaterial(0);
	}

	if (MaterialToUse)
	{
		Mesh->SetMaterial(0, MaterialToUse);
		MID = Mesh->CreateDynamicMaterialInstance(0, MaterialToUse);
	}

	RefreshPreviewVisual();
}
#endif

void ATerritoryChunkActor::SetPreviewMode(bool bInPreviewMode)
{
	bPreviewMode = bInPreviewMode;

	if (Mesh)
	{
		Mesh->SetMobility(bPreviewMode ? EComponentMobility::Movable : EComponentMobility::Static);
		Mesh->SetCollisionEnabled(bPreviewMode ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
	}

	RefreshPreviewVisual();
}

void ATerritoryChunkActor::SetPreviewValid(bool bInPreviewValid)
{
	bPreviewValid = bInPreviewValid;
	RefreshPreviewVisual();
}

void ATerritoryChunkActor::RefreshPreviewVisual()
{
	if (!Mesh)
	{
		return;
	}

	if (!bPreviewMode)
	{
		Mesh->SetOverlayMaterial(nullptr);
		if (MID)
		{
			MID->SetVectorParameterValue(TEXT("TintColor"), FLinearColor::White);
			MID->SetScalarParameterValue(TEXT("Opacity"), 1.f);
		}
		return;
	}

	const FLinearColor ValidColor(0.45f, 0.85f, 1.0f, 1.f);
	const FLinearColor InvalidColor(1.f, 0.35f, 0.35f, 1.f);
	const FLinearColor PreviewColor = bPreviewValid ? ValidColor : InvalidColor;
	const float PreviewOpacity = bPreviewValid ? 0.4f : 0.5f;

	if (MID)
	{
		MID->SetVectorParameterValue(TEXT("TintColor"), PreviewColor);
		MID->SetScalarParameterValue(TEXT("Opacity"), PreviewOpacity);
	}

	if (!PreviewOverlayMID && PreviewOverlayMaterial)
	{
		PreviewOverlayMID = UMaterialInstanceDynamic::Create(PreviewOverlayMaterial, this);
	}

	if (PreviewOverlayMID)
	{
		PreviewOverlayMID->SetVectorParameterValue(TEXT("Color"), PreviewColor);
		PreviewOverlayMID->SetVectorParameterValue(TEXT("BaseColor"), PreviewColor);
		PreviewOverlayMID->SetVectorParameterValue(TEXT("TintColor"), PreviewColor);
		PreviewOverlayMID->SetVectorParameterValue(TEXT("EmissiveColor"), PreviewColor);
		PreviewOverlayMID->SetScalarParameterValue(TEXT("Opacity"), PreviewOpacity);
		Mesh->SetOverlayMaterial(PreviewOverlayMID);
	}
}

