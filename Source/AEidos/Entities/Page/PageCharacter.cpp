// Fill out your copyright notice in the Description page of Project Settings.


#include "Entities/Page/PageCharacter.h"
#include "Combat/WS_CombatDirector.h"
#include "Entities/Page/Components/StatsComponent.h"
#include "Entities/Page/Components/SkillComponent.h"
#include "Entities/Items/InventoryComponent.h"
#include "Framework/EidosPlayerController.h"
#include "Player/Camera/CameraModeComponent.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

// Sets default values
APageCharacter::APageCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> SphereMaterialFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	Stats = CreateDefaultSubobject<UStatsComponent>(TEXT("StatsComponent"));
	Skills = CreateDefaultSubobject<USkillComponent>(TEXT("SkillComponent"));
	Inventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
	
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
	SpringArm->bDoCollisionTest = false; // ?먰븯硫?耳쒕룄 ??
	SpringArm->SetRelativeRotation(FRotator::ZeroRotator);

	ThirdPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ThirdPersonCamera"));
	ThirdPersonCamera->SetupAttachment(SpringArm);
	ThirdPersonCamera->bUsePawnControlRotation = false;

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetRootComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(20.f, 0.f, 70.f));
	FirstPersonCamera->bUsePawnControlRotation = true;

	CombatIndicatorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CombatIndicatorMesh"));
	CombatIndicatorMesh->SetupAttachment(GetRootComponent());
	CombatIndicatorMesh->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
	CombatIndicatorMesh->SetRelativeScale3D(FVector(0.18f));
	CombatIndicatorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CombatIndicatorMesh->SetGenerateOverlapEvents(false);
	CombatIndicatorMesh->SetCastShadow(false);
	CombatIndicatorMesh->SetHiddenInGame(true);
	CombatIndicatorMesh->SetVisibility(false);
	CombatIndicatorMesh->SetReceivesDecals(false);
	CombatIndicatorMesh->SetCanEverAffectNavigation(false);

	if (SphereMeshFinder.Succeeded())
	{
		CombatIndicatorMesh->SetStaticMesh(SphereMeshFinder.Object);
	}

	if (SphereMaterialFinder.Succeeded())
	{
		CombatIndicatorMesh->SetMaterial(0, SphereMaterialFinder.Object);
	}

	SetViewMode(EPageViewMode::ThirdPerson);

	CombatActionSlots.SetNum(10);
	DefaultSkillIds.Add(TEXT("Slash"));

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

	for (const FName SkillId : DefaultSkillIds)
	{
		GrantSkill(SkillId);
	}
	EnsureDefaultCombatLoadout();
	if (Inventory)
	{
		Inventory->SetCapacity(MaxInventoryWeight, MaxInventoryVolume);
		Inventory->OnInventoryChanged.AddDynamic(this, &APageCharacter::HandleInventoryChanged);
	}

	PreviousWorldLocation = GetActorLocation();

	if (CombatIndicatorMesh)
	{
		if (UMaterialInstanceDynamic* IndicatorMID = CombatIndicatorMesh->CreateAndSetMaterialInstanceDynamic(0))
		{
			const FLinearColor RedTint(1.0f, 0.1f, 0.1f, 1.0f);
			IndicatorMID->SetVectorParameterValue(TEXT("Color"), RedTint);
			IndicatorMID->SetVectorParameterValue(TEXT("BaseColor"), RedTint);
			IndicatorMID->SetVectorParameterValue(TEXT("EmissiveColor"), RedTint * 8.f);
		}
	}
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
			const float Yaw = PC->GetCameraMode()->GetOrbitYawWorldDeg(); // getter 留뚮뱾湲?
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

	// Work??諛섎뱶??FixedTick 湲곗??쇰줈 ?몄텧
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

void APageCharacter::GrantSkill(FName SkillId)
{
	if (Skills && !SkillId.IsNone())
	{
		Skills->GrantSkill(SkillId);
	}
}

void APageCharacter::EnsureDefaultCombatLoadout()
{
	if (!IsFriendly())
	{
		return;
	}

	bool bHasAssignedAction = false;
	for (const FPageCombatActionSlot& Slot : CombatActionSlots)
	{
		if (Slot.ActionType != EPageCombatActionType::None)
		{
			bHasAssignedAction = true;
			break;
		}
	}

	if (bHasAssignedAction)
	{
		return;
	}

	const FName DefaultSkillId(TEXT("Slash"));
	GrantSkill(DefaultSkillId);

	FPageCombatActionSlot SlashSlot;
	SlashSlot.ActionType = EPageCombatActionType::ActiveSkill;
	SlashSlot.ActionId = DefaultSkillId;
	SlashSlot.DisplayName = FText::FromString(TEXT("Slash"));
	SetCombatActionSlot(0, SlashSlot);
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

	if (CombatIndicatorMesh)
	{
		CombatIndicatorMesh->SetHiddenInGame(!bInTurnCombat);
		CombatIndicatorMesh->SetVisibility(bInTurnCombat, true);
	}

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

void APageCharacter::SetInventorySummary(float InCurrentVolume, float InMaxVolume, float InCurrentWeight, float InMaxWeight)
{
	MaxInventoryVolume = FMath::Max(InCurrentVolume, InMaxVolume);
	MaxInventoryWeight = FMath::Max(InCurrentWeight, InMaxWeight);
	if (Inventory)
	{
		Inventory->SetCapacity(MaxInventoryWeight, MaxInventoryVolume);
	}
	else
	{
		CurrentInventoryVolume = FMath::Max(0.f, InCurrentVolume);
		CurrentInventoryWeight = FMath::Max(0.f, InCurrentWeight);
	}
}

float APageCharacter::GetCurrentInventoryVolume() const
{
	return Inventory ? Inventory->GetCurrentVolume() : CurrentInventoryVolume;
}

float APageCharacter::GetMaxInventoryVolume() const
{
	return Inventory ? Inventory->GetMaxVolume() : MaxInventoryVolume;
}

float APageCharacter::GetCurrentInventoryWeight() const
{
	return Inventory ? Inventory->GetCurrentWeight() : CurrentInventoryWeight;
}

float APageCharacter::GetMaxInventoryWeight() const
{
	return Inventory ? Inventory->GetMaxWeight() : MaxInventoryWeight;
}

void APageCharacter::HandleInventoryChanged()
{
	CurrentInventoryVolume = Inventory ? Inventory->GetCurrentVolume() : CurrentInventoryVolume;
	CurrentInventoryWeight = Inventory ? Inventory->GetCurrentWeight() : CurrentInventoryWeight;
}

