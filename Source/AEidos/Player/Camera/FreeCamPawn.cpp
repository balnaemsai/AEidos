// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/Camera/FreeCamPawn.h"

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

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(GetRootComponent());
	SpringArm->TargetArmLength = 0.f;              // ✅ “카메라가 바로 붙는” 1인칭형 freecam
	SpringArm->bUsePawnControlRotation = true;     // ✅ 핵심

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);

	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = true;
	bUseControllerRotationRoll = false;

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

	const FRotator YawRot(0.f, GetActorRotation().Yaw, 0.f);
	const FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	const FVector Right   = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

	AddMovementInput(Forward, V.Y);
	AddMovementInput(Right,   V.X);
}
