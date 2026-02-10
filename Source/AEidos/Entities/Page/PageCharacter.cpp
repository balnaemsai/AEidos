// Fill out your copyright notice in the Description page of Project Settings.


#include "Entities/Page/PageCharacter.h"
#include "Entities/Page/Components/StatsComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

// Sets default values
APageCharacter::APageCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	Stats = CreateDefaultSubobject<UStatsComponent>(TEXT("StatsComponent"));
	
	bUseControllerRotationYaw = false;
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = true;
		MoveComp->RotationRate = FRotator(0.f, 540.f, 0.f);
		MoveComp->MaxWalkSpeed = 450.f;
	}

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(GetRootComponent());
	SpringArm->bUsePawnControlRotation = true;

	// ✅ 기본 쿼터뷰 느낌: 각도/거리
	SpringArm->TargetArmLength = 700.f;
	SpringArm->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
	SpringArm->SetRelativeRotation(FRotator(-45.f, 0.f, 0.f));

	ThirdPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ThirdPersonCamera"));
	ThirdPersonCamera->SetupAttachment(SpringArm);
	ThirdPersonCamera->bUsePawnControlRotation = false;

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetRootComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(20.f, 0.f, 70.f));
	FirstPersonCamera->bUsePawnControlRotation = true;

	SetViewMode(EPageViewMode::ThirdPerson);

}

void APageCharacter::SetViewMode(EPageViewMode NewMode)
{
	ViewMode = NewMode;
	const bool bFirst = (ViewMode == EPageViewMode::FirstPerson);

	FirstPersonCamera->SetActive(bFirst);
	ThirdPersonCamera->SetActive(!bFirst);
}

void APageCharacter::ToggleViewMode()
{
	SetViewMode(ViewMode == EPageViewMode::ThirdPerson ? EPageViewMode::FirstPerson : EPageViewMode::ThirdPerson);
}

void APageCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
				LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				Subsystem->AddMappingContext(PageInputMappingContext, 0);
			}
		}
	}
}

// Called to bind functionality to input
void APageCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EIComp = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	
	EIComp->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APageCharacter::HandleMove);
}

void APageCharacter::HandleMove(const FInputActionValue& Value)
{
	const FVector2D MoveInput = Value.Get<FVector2D>();

	if (MoveInput.IsNearlyZero())
		return;

	const FRotator YawRot(0.f, Controller->GetControlRotation().Yaw, 0.f);

	const FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	const FVector Right   = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

	AddMovementInput(Forward, MoveInput.Y);
	AddMovementInput(Right,   MoveInput.X);
}