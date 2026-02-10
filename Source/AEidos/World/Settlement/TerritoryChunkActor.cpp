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

	if (TerritoryMaterial)
	{
		Mesh->SetMaterial(0, TerritoryMaterial);
		MID = Mesh->CreateDynamicMaterialInstance(0, TerritoryMaterial);
	}
}

