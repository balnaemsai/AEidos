// Fill out your copyright notice in the Description page of Project Settings.


#include "World/Settlement/Portal/PortalActor.h"

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Entities/Page/PageCharacter.h"
#include "Framework/EidosPlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "World/Settlement/WS_PortalDirector.h"

APortalActor::APortalActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	PortalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalMesh"));
	PortalMesh->SetupAttachment(Root);
	PortalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(Root);
	InteractionSphere->SetSphereRadius(InteractionRadius);
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void APortalActor::BeginPlay()
{
	Super::BeginPlay();

	if (InteractionSphere)
	{
		InteractionSphere->SetSphereRadius(InteractionRadius);
	}
}

void APortalActor::InitializePortal(int32 InPortalId, int32 InTier)
{
	PortalId = InPortalId;
	Tier = InTier;

	UE_LOG(LogTemp, Log,
		TEXT("[PortalActor] InitializePortal PortalId=%d Tier=%d"),
		PortalId,
		Tier);

	BP_OnPortalInitialized();
}

void APortalActor::Interact(APlayerController* InteractingPC)
{
	if (PortalId == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PortalActor] Interact failed: PortalId invalid"));
		return;
	}

	if (!GetWorld())
	{
		return;
	}

	APageCharacter* InteractingPage = nullptr;
	if (InteractingPC)
	{
		if (APawn* Pawn = InteractingPC->GetPawn())
		{
			InteractingPage = Cast<APageCharacter>(Pawn);
		}

		if (!InteractingPage)
		{
			if (const AEidosPlayerController* EidosPC = Cast<AEidosPlayerController>(InteractingPC))
			{
				InteractingPage = EidosPC->GetSelectedPage();
			}
		}

		if (InteractingPage)
		{
			const float DistSq = FVector::DistSquared(InteractingPage->GetActorLocation(), GetActorLocation());
			if (DistSq > FMath::Square(InteractionRadius))
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[PortalActor] Interact failed: too far PortalId=%d Dist=%.1f"),
					PortalId,
					FMath::Sqrt(DistSq));
				return;
			}
		}
	}

	UWS_PortalDirector* PortalDirector = GetWorld()->GetSubsystem<UWS_PortalDirector>();
	if (!PortalDirector)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PortalActor] Interact failed: PortalDirector missing"));
		return;
	}

	const bool bSuccess = PortalDirector->RequestEnterPortal(PortalId, InteractingPage);

	UE_LOG(LogTemp, Log,
		TEXT("[PortalActor] Interact PortalId=%d Result=%s"),
		PortalId,
		bSuccess ? TEXT("Success") : TEXT("Failed"));
}
