#include "World/Dungeon/DungeonReturnPortalActor.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Entities/Page/PageCharacter.h"
#include "Framework/EidosPlayerController.h"
#include "UObject/ConstructorHelpers.h"
#include "World/Dungeon/WS_DungeonRuntime.h"

ADungeonReturnPortalActor::ADungeonReturnPortalActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	PortalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalMesh"));
	PortalMesh->SetupAttachment(Root);
	PortalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PortalMesh->SetRelativeScale3D(FVector::OneVector);

	// Use the project portal mesh rather than an engine placeholder so the
	// collapse exit reads as the same kind of dimensional portal as settlement entry.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PortalMeshAsset(
		TEXT("/Game/Assets/Meshes/SM_Portal_Test.SM_Portal_Test"));
	if (PortalMeshAsset.Succeeded())
	{
		PortalMesh->SetStaticMesh(PortalMeshAsset.Object);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[DungeonReturnPortal] Missing SM_Portal_Test mesh asset"));
	}

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(Root);
	InteractionSphere->SetSphereRadius(InteractionRadius);
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	PortalLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PortalLight"));
	PortalLight->SetupAttachment(Root);
	PortalLight->SetIntensity(3500.f);
	PortalLight->SetLightColor(FLinearColor(0.82f, 0.76f, 0.62f));
	PortalLight->SetAttenuationRadius(500.f);
	PortalLight->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
}

void ADungeonReturnPortalActor::Interact(APlayerController* InteractingController)
{
	AEidosPlayerController* EidosPC = Cast<AEidosPlayerController>(InteractingController);
	APageCharacter* SelectedPage = EidosPC ? EidosPC->GetSelectedPage() : nullptr;
	if (!SelectedPage || !SelectedPage->IsInDungeon())
	{
		return;
	}

	const float DistanceSq = FVector::DistSquared(SelectedPage->GetActorLocation(), GetActorLocation());
	if (DistanceSq > FMath::Square(InteractionRadius))
	{
		UE_LOG(LogTemp, Warning, TEXT("[DungeonReturnPortal] Interact failed: PageId=%d is too far"), SelectedPage->GetPageEntityId());
		return;
	}

	UWS_DungeonRuntime* DungeonRuntime = GetWorld() ? GetWorld()->GetSubsystem<UWS_DungeonRuntime>() : nullptr;
	if (!DungeonRuntime || !DungeonRuntime->ReturnPageFromActiveDungeon(SelectedPage))
	{
		UE_LOG(LogTemp, Warning, TEXT("[DungeonReturnPortal] Interact failed: return rejected for PageId=%d"), SelectedPage->GetPageEntityId());
	}
}
