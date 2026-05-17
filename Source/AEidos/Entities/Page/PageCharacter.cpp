// Fill out your copyright notice in the Description page of Project Settings.


#include "Entities/Page/PageCharacter.h"
#include "Combat/WS_CombatDirector.h"
#include "Entities/Page/Components/StatsComponent.h"
#include "Entities/Page/Components/SkillComponent.h"
#include "Framework/EidosPlayerController.h"
#include "Player/Camera/CameraModeComponent.h"

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
	Skills = CreateDefaultSubobject<USkillComponent>(TEXT("SkillComponent"));
	
	bUseControllerRotationYaw = false;
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = true;
		MoveComp->RotationRate = FRotator(0.f, 540.f, 0.f);
		MoveComp->MaxWalkSpeed = 450.f;
	}

	ThirdPersonPivot = CreateDefaultSubobject<USceneComponent>(TEXT("ThirdPersonPivot"));
	ThirdPersonPivot->SetupAttachment(GetRootComponent());
	ThirdPersonPivot->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
	ThirdPersonPivot->SetRelativeRotation(FRotator(-45.f, 0.f, 0.f));

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(ThirdPersonPivot);
	
	SpringArm->bUsePawnControlRotation = false;
	SpringArm->bInheritYaw   = true;
	SpringArm->bInheritPitch = true;
	SpringArm->bInheritRoll  = false;
	
	SpringArm->TargetArmLength = 700.f;
	SpringArm->bDoCollisionTest = false; // 원하면 켜도 됨
	SpringArm->SetRelativeRotation(FRotator::ZeroRotator);

	ThirdPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ThirdPersonCamera"));
	ThirdPersonCamera->SetupAttachment(SpringArm);
	ThirdPersonCamera->bUsePawnControlRotation = false;

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetRootComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(20.f, 0.f, 70.f));
	FirstPersonCamera->bUsePawnControlRotation = true;

	SetViewMode(EPageViewMode::ThirdPerson);

	CombatActionSlots.SetNum(10);
	CombatActionSlots[9].ActionType = EPageCombatActionType::EndTurn;
	CombatActionSlots[9].DisplayName = FText::FromString(TEXT("End Turn"));

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

	PreviousWorldLocation = GetActorLocation();
}

void APageCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const FVector CurrentLocation = GetActorLocation();
	if (bInTurnCombat && bHasActiveCombatTurn && IsFriendly())
	{
		const float TravelDistanceCm = FVector::Dist2D(CurrentLocation, PreviousWorldLocation);
		if (TravelDistanceCm > 0.5f)
		{
			if (UWS_CombatDirector* CombatDirector = GetWorld() ? GetWorld()->GetSubsystem<UWS_CombatDirector>() : nullptr)
			{
				if (!CombatDirector->NotifyPageMoved(this, TravelDistanceCm))
				{
					if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
					{
						MoveComp->StopMovementImmediately();
					}
				}
			}
		}
	}

	PreviousWorldLocation = CurrentLocation;
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
	if (bInTurnCombat && !bHasActiveCombatTurn)
	{
		return;
	}

	const FVector2D MoveInput = Value.Get<FVector2D>();

	if (MoveInput.IsNearlyZero())
		return;

	AEidosPlayerController* PC = Cast<AEidosPlayerController>(GetController());

	if (ViewMode == EPageViewMode::ThirdPerson)
	{
		
		if (PC && PC->GetCameraMode())
		{
			const float Yaw = PC->GetCameraMode()->GetOrbitYawWorldDeg(); // getter 만들기
			const FRotator YawRot(0.f, Yaw, 0.f);

			const FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
			const FVector Right   = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

			AddMovementInput(Forward, MoveInput.Y);
			AddMovementInput(Right,   MoveInput.X);
			return;
		}
	}
	
	const FRotator ControlRot(0.f, PC->GetControlRotation().Yaw, 0.f);
	const FVector Forward = FRotationMatrix(ControlRot).GetUnitAxis(EAxis::X);
	const FVector Right   = FRotationMatrix(ControlRot).GetUnitAxis(EAxis::Y);
	AddMovementInput(Forward, MoveInput.Y);
	AddMovementInput(Right,   MoveInput.X);
}

void APageCharacter::AddWorkSkillXP(FName SkillId, float WorkRatePerSecond, float FixedDeltaSeconds, float XPFactor)
{
	if (!Skills || SkillId.IsNone())
	{
		return;
	}

	// Work는 반드시 FixedTick 기준으로 호출
	Skills->AddContinuousSkillXP(SkillId, WorkRatePerSecond, FixedDeltaSeconds, XPFactor);
}

void APageCharacter::AddMovementSkillXP(FName SkillId, float DistanceCm, float XPPerCm)
{
	if (!Skills || SkillId.IsNone() || DistanceCm <= 0.f || XPPerCm <= 0.f)
	{
		return;
	}

	const float XP = DistanceCm * XPPerCm;
	Skills->AddSkillXP(SkillId, XP);
}

void APageCharacter::AddCombatSkillXP(FName SkillId, float FlatXP)
{
	if (!Skills || SkillId.IsNone() || FlatXP <= 0.f)
	{
		return;
	}

	Skills->AddDiscreteSkillXP(SkillId, FlatXP);
}

void APageCharacter::AddActiveSkillXP(FName SkillId, float FlatXP)
{
	if (!Skills || SkillId.IsNone() || FlatXP <= 0.f)
	{
		return;
	}

	Skills->AddDiscreteSkillXP(SkillId, FlatXP);
}

void APageCharacter::GainSkillXP(FName SkillId, float Amount, bool bPropagate)
{
	if (!Skills || SkillId.IsNone() || Amount <= 0.f)
	{
		return;
	}

	if (bPropagate)
	{
		Skills->AddSkillXP(SkillId, Amount);
	}
	else
	{
		Skills->AddSkillXP_NoPropagation(SkillId, Amount);
	}
}

float APageCharacter::GetSkillMultiplier(FName SkillId) const
{
	if (!Skills)
	{
		return 1.f;
	}

	return Skills->GetSkillMultiplier(SkillId);
}

int32 APageCharacter::GetSkillLevel(FName SkillId) const
{
	if (!Skills)
	{
		return 0;
	}

	return Skills->GetSkillLevel(SkillId);
}

void APageCharacter::SetPageEntityId(int32 NewPageEntityId)
{
	PageEntityId = FMath::Max(0, NewPageEntityId);
}

void APageCharacter::SetFaction(EPageFaction NewFaction)
{
	Faction = NewFaction;
}

bool APageCharacter::IsHostileTo(const APageCharacter* OtherPage) const
{
	return OtherPage && OtherPage != this && Faction != OtherPage->Faction;
}

void APageCharacter::SetIsInDungeon(bool bNewIsInDungeon)
{
	bIsInDungeon = bNewIsInDungeon;
}

void APageCharacter::SetTurnCombatState(bool bNewInTurnCombat, bool bNewHasActiveCombatTurn)
{
	const bool bWasInTurnCombat = bInTurnCombat;
	const bool bWasActiveTurn = bHasActiveCombatTurn;

	bInTurnCombat = bNewInTurnCombat;
	bHasActiveCombatTurn = bNewHasActiveCombatTurn;

	if ((bInTurnCombat && (!bHasActiveCombatTurn || !bWasInTurnCombat || !bWasActiveTurn))
		|| (!bInTurnCombat && bWasInTurnCombat))
	{
		if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
		{
			MoveComp->StopMovementImmediately();
		}
	}

	PreviousWorldLocation = GetActorLocation();
}

bool APageCharacter::GetCombatActionSlot(int32 SlotIndex, FPageCombatActionSlot& OutSlot) const
{
	if (!CombatActionSlots.IsValidIndex(SlotIndex))
	{
		return false;
	}

	OutSlot = CombatActionSlots[SlotIndex];
	return true;
}

void APageCharacter::SetCombatActionSlot(int32 SlotIndex, const FPageCombatActionSlot& InSlot)
{
	if (SlotIndex < 0)
	{
		return;
	}

	if (CombatActionSlots.Num() <= SlotIndex)
	{
		CombatActionSlots.SetNum(SlotIndex + 1);
	}

	CombatActionSlots[SlotIndex] = InSlot;
}
