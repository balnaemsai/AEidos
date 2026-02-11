// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/Camera/FreeCamPawn.h"

#include "CameraModeComponent.h"
#include "Entities/Page/PageCharacter.h"
#include "Framework/EidosPlayerController.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "EnhancedInputComponent.h"

// Sets default values
AFreeCamPawn::AFreeCamPawn()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	ThirdPersonPivot = CreateDefaultSubobject<USceneComponent>(TEXT("ThirdPersonPivot"));
	ThirdPersonPivot->SetupAttachment(Root);
	ThirdPersonPivot->SetRelativeLocation(FVector(0,0,0));
	ThirdPersonPivot->SetRelativeRotation(FRotator(0,0,0));

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(ThirdPersonPivot);
	SpringArm->bUsePawnControlRotation = false;   // ✅ Pivot/Orbit로 돌릴 거라 false
	SpringArm->bInheritYaw   = false;
	SpringArm->bInheritPitch = false;
	SpringArm->bInheritRoll  = false;

	SpringArm->TargetArmLength = 700.f;           // ✅ 3인칭 줌 대상
	SpringArm->bDoCollisionTest = false;
	SpringArm->SetRelativeRotation(FRotator::ZeroRotator);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);
	Camera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw   = false;          // ✅ freecam 이동은 카메라 오빗과 분리
	bUseControllerRotationPitch = false;

	Movement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
	Movement->MaxSpeed = 1600.f;
}

// Called to bind functionality to input
void AFreeCamPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EI = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	EI->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AFreeCamPawn::HandleMove);
}

void AFreeCamPawn::HandleMove(const FInputActionValue& Value)
{
	const FVector2D V = Value.Get<FVector2D>();
	if (V.IsNearlyZero()) return;

	AEidosPlayerController* PC = Cast<AEidosPlayerController>(GetController());
	if (!PC || !PC->GetCameraMode()) return;

	const bool bThirdPerson = (PC->GetCameraMode()->GetViewMode() == EPageViewMode::ThirdPerson);

	const float Yaw = bThirdPerson
		? PC->GetCameraMode()->GetOrbitYawWorldDeg()
		: PC->GetControlRotation().Yaw;

	const FRotator YawRot(0.f, Yaw, 0.f);
	const FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	const FVector Right   = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

	AddMovementInput(Forward, V.Y);
	AddMovementInput(Right,   V.X);
}

void AFreeCamPawn::ApplyViewMode(EPageViewMode Mode)
{
	const bool bFirst = (Mode == EPageViewMode::FirstPerson);

	SpringArm->bUsePawnControlRotation = bFirst;
	Camera->bUsePawnControlRotation = bFirst;  

	bUseControllerRotationYaw   = bFirst;
	bUseControllerRotationPitch = bFirst;

}
