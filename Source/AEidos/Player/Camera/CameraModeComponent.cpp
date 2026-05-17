// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/Camera/CameraModeComponent.h"

#include "Framework/EidosPlayerController.h"
#include "Entities/Page/PageCharacter.h"
#include "FreeCamPawn.h"

#include "EnhancedInputSubsystems.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Engine/Level.h"

UCameraModeComponent::UCameraModeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.SetTickFunctionEnable(true);
}

void UCameraModeComponent::OnRegister()
{
	Super::OnRegister();

	// ✅ 등록 타이밍에 “틱 가능 + 틱 활성”을 확정
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	SetComponentTickEnabled(true);

	// (선택) 디버그
	UE_LOG(LogTemp, Warning, TEXT("[CamMode OnRegister] Reg=%d Active=%d CanEver=%d TickEnabled=%d"),
		IsRegistered(), IsActive(),
		PrimaryComponentTick.bCanEverTick,
		IsComponentTickEnabled());
}

void UCameraModeComponent::BeginPlay()
{
	Super::BeginPlay();

	Activate(true);

	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	SetComponentTickEnabled(true);
	PrimaryComponentTick.SetTickFunctionEnable(true);
	
	if (USpringArmComponent* Arm = ResolveActiveThirdPersonArm())
	{
		DesiredArmLength = Arm->TargetArmLength;
		DesiredPitch = Arm->GetRelativeRotation().Pitch;
	}
}

void UCameraModeComponent::AddOrbitYawInput(float Axis)
{
	// 3인칭일 때만
	if (GetViewMode() != EPageViewMode::ThirdPerson)
		return;

	UE_LOG(LogTemp, Warning, TEXT("[CMC] AddOrbitYawInput."));
	PendingYawAxis += Axis;
}

void UCameraModeComponent::AddZoomInput(float Axis)
{
	// 3인칭일 때만
	if (GetViewMode() != EPageViewMode::ThirdPerson)
		return;

	UE_LOG(LogTemp, Warning, TEXT("[CMC] AddZoom."));
	PendingZoomAxis += Axis;
}

USpringArmComponent* UCameraModeComponent::ResolveActiveThirdPersonArm() const
{
	if (ControlMode == ECameraControlMode::FollowPage)
	{
		if (APageCharacter* Page = SelectedPage.Get())
		{
			return Page->GetThirdPersonSpringArm(); 
		}
		return nullptr;
	}

	if (ControlMode == ECameraControlMode::FreeCam)
	{
		if (AFreeCamPawn* Cam = FreeCamPawn.Get())
		{
			return Cam->GetThirdPersonSpringArm(); 
		}
		return nullptr;
	}

	return nullptr;
}

void UCameraModeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	USpringArmComponent* Arm = ResolveActiveThirdPersonArm();
	USceneComponent* Pivot = ResolveActiveThirdPersonPivot();
	
	if (!Arm || !Pivot) return;
	
	if (!FMath::IsNearlyZero(PendingYawAxis))
	{
		OrbitYawWorldDeg += PendingYawAxis * OrbitYawSpeedDegPerSec * DeltaTime;
		UE_LOG(LogTemp, Warning, TEXT("[CMC-Tick] OrbitYawWorldDeg is %f"), OrbitYawWorldDeg);
		PendingYawAxis = 0.f;
	}
	
	if (!FMath::IsNearlyZero(PendingZoomAxis))
	{
		DesiredArmLength = FMath::Clamp(
			DesiredArmLength - PendingZoomAxis * ZoomStep,
			ArmLengthMin, ArmLengthMax);

		const float Alpha = (DesiredArmLength - ArmLengthMin) / (ArmLengthMax - ArmLengthMin);
		DesiredPitch = FMath::Lerp(PitchAtZoomIn, PitchAtZoomOut, Alpha);

		PendingZoomAxis = 0.f;
	}

	// 3) 부드럽게 적용: ArmLength
	Arm->TargetArmLength = FMath::FInterpTo(Arm->TargetArmLength, DesiredArmLength, DeltaTime, ArmInterpSpeed);

	// 4) 부드럽게 적용: Pivot Pitch, Pivot Yaw(오빗)
	FRotator PR = Pivot->GetComponentRotation();          // 월드 회전으로 통일
	float NewPitch = FMath::FInterpTo(PR.Pitch, DesiredPitch, DeltaTime, PitchInterpSpeed);
	float NewYaw   = OrbitYawWorldDeg;

	Pivot->SetWorldRotation(FRotator(NewPitch, NewYaw, 0.f));

	USceneComponent* Parent = Arm->GetAttachParent();
}

void UCameraModeComponent::InitializeForController(AEidosPlayerController* InPC)
{
	PC = InPC;

	// 기본: FollowPage
	ControlMode = ECameraControlMode::FollowPage;
}

void UCameraModeComponent::SetSelectedPage(APageCharacter* NewPage)
{
	UE_LOG(LogTemp, Warning, TEXT("[CamMode] SetSelectedPage: New=%s  Old=%s"),
		*GetNameSafe(NewPage),
		*GetNameSafe(SelectedPage.Get()));

	UE_LOG(LogTemp, Warning, TEXT("[CamMode] SetSelectedPage this=%p"), this);

	SelectedPage = NewPage;

	if (ControlMode == ECameraControlMode::FollowPage)
	{
		EnterFollowPage();
	}
}

void UCameraModeComponent::FocusSelectedPage(bool bForceFollowMode)
{
	if (!SelectedPage)
	{
		return;
	}

	if (bForceFollowMode && ControlMode != ECameraControlMode::FollowPage)
	{
		ControlMode = ECameraControlMode::FollowPage;
	}

	EnterFollowPage();
}

void UCameraModeComponent::ToggleViewMode()
{
	if (ControlMode != ECameraControlMode::FollowPage)
	{
		return;
	}

	if (APageCharacter* Page = SelectedPage.Get())
	{
		Page->ToggleViewMode();

		if (ControlMode == ECameraControlMode::FreeCam)
		{
			if (AFreeCamPawn* Cam = FreeCamPawn.Get())
			{
				Cam->ApplyViewMode(Page->GetViewMode());
			}
		}
	}
}

void UCameraModeComponent::ToggleControlMode()
{
	UE_LOG(LogTemp, Display, TEXT("ToggleControlMode"));
	if (ControlMode == ECameraControlMode::FollowPage)
	{
		ControlMode = ECameraControlMode::FreeCam;
		EnterFreeCam();
	}
	else
	{
		ControlMode = ECameraControlMode::FollowPage;
		EnterFollowPage();
	}
}

void UCameraModeComponent::EnterFollowPage()
{
	AEidosPlayerController* C = PC.Get();
	if (!C)
	{
		return;
	}

	APageCharacter* Page = SelectedPage.Get();
	if (!Page)
	{
		return;
	}

	// FreeCam에서 왔다면 FreeCam IMC 제거
	if (AFreeCamPawn* CamPawn = FreeCamPawn.Get())
	{
		RemoveIMC(CamPawn->GetFreeCamIMC());
	}

	if (UInputMappingContext* PageIMC = Page->GetPageIMC())
	{
		AddIMC(PageIMC, 1);
	}

	C->Possess(Page);
	C->SetViewTargetWithBlend(Page, 0.15f);
	

	if (GetViewMode() == EPageViewMode::ThirdPerson)
	{
		EnterThirdPerson(true);
	}
	
}

void UCameraModeComponent::EnterFreeCam()
{
	AEidosPlayerController* C = PC.Get();
	if (!C)
	{
		return;
	}

	UWorld* W = GetWorld();
	if (!W)
	{
		return;
	}

	if (!FreeCamPawn.IsValid())
	{
		TSubclassOf<AFreeCamPawn> SpawnClass = FreeCamPawnClass;
		if (!SpawnClass)
		{
			SpawnClass = AFreeCamPawn::StaticClass();
		}

		const FVector BaseLoc = (SelectedPage->GetActorLocation());

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AFreeCamPawn* Spawned = W->SpawnActor<AFreeCamPawn>(SpawnClass, BaseLoc, C->GetControlRotation(), Params);
		FreeCamPawn = Spawned;
	}

	if (APageCharacter* Page = SelectedPage.Get())
	{
		RemoveIMC(Page->GetPageIMC());
	}

	if (AFreeCamPawn* CamPawn = FreeCamPawn.Get())
	{
		C->Possess(CamPawn);
		C->SetViewTargetWithBlend(CamPawn, 0.15f);
		AddIMC(CamPawn->GetFreeCamIMC(), 1);
		CamPawn->ApplyViewMode(GetViewMode());

		if (GetViewMode() == EPageViewMode::ThirdPerson)
		{
			EnterThirdPerson(true);
		}
	}
}

void UCameraModeComponent::AddIMC(UInputMappingContext* IMC, int32 Priority)
{
	AEidosPlayerController* C = PC.Get();
	if (!C || !IMC)
	{
		return;
	}

	if (ULocalPlayer* LP = C->GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Sub = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Sub->AddMappingContext(IMC, Priority);
		}
	}
}

void UCameraModeComponent::RemoveIMC(UInputMappingContext* IMC)
{
	AEidosPlayerController* C = PC.Get();
	if (!C || !IMC)
	{
		return;
	}

	if (ULocalPlayer* LP = C->GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Sub = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Sub->RemoveMappingContext(IMC);
		}
	}
}

void UCameraModeComponent::EnterThirdPerson(bool bForceApply)
{
	USpringArmComponent* Arm = ResolveActiveThirdPersonArm();
	USceneComponent* Pivot = ResolveActiveThirdPersonPivot();

	UE_LOG(LogTemp, Warning, TEXT("[EnterThirdPerson] Arm=%s Pivot=%s"),
		*GetNameSafe(Arm), *GetNameSafe(Pivot));
	if (!Arm || !Pivot) return;

	Arm->bUsePawnControlRotation = false;
	Arm->bInheritYaw = true;
	Arm->bInheritPitch = true;
	Arm->bInheritRoll = false;
	Pivot->SetUsingAbsoluteRotation(false);
	Arm->SetUsingAbsoluteRotation(false);
	
	if (bForceApply)
	{
		DesiredArmLength = 700.f;
		DesiredPitch     = -45.f;

		OrbitYawWorldDeg = Pivot->GetComponentRotation().Yaw; // ✅ Pivot 기준
		Arm->TargetArmLength = DesiredArmLength;

		FRotator PR = Pivot->GetComponentRotation();
		PR.Pitch = DesiredPitch;
		Pivot->SetWorldRotation(FRotator(PR.Pitch, OrbitYawWorldDeg, 0.f));
	}
	else
	{
		DesiredArmLength = Arm->TargetArmLength;
		DesiredPitch     = Pivot->GetComponentRotation().Pitch;
		OrbitYawWorldDeg = Pivot->GetComponentRotation().Yaw;
	}
}

USceneComponent* UCameraModeComponent::ResolveActiveThirdPersonPivot() const
{
	if (ControlMode == ECameraControlMode::FollowPage)
	{
		if (APageCharacter* Page = SelectedPage.Get())
		{
			return Page->GetThirdPersonPivot();
		}
	}
	else if (ControlMode == ECameraControlMode::FreeCam)
	{
		if (AFreeCamPawn* Cam = FreeCamPawn.Get())
		{
			return Cam->GetThirdPersonPivot(); // FreeCamPawn에도 Pivot 만들어두는 방식 추천
		}
	}
	return nullptr;
}

EPageViewMode UCameraModeComponent::GetViewMode() const
{
	if (ControlMode == ECameraControlMode::FollowPage)
	{
		if (APageCharacter* Page = SelectedPage.Get())
		{
			return Page->GetViewMode();
		}
		return EPageViewMode::ThirdPerson;
	}

	/*

	if (ControlMode == ECameraControlMode::FreeCam)
	{
		if (AFreeCamPawn* Cam = FreeCamPawn.Get())
		{
			return Cam->GetViewMode();
		}
		return EPageViewMode::ThirdPerson;
	}
	지금 FollowPage일때랑 Freecam일때 각각 어떻게 작동하는지 정리해봐야할듯. 이거 Viewmode를 보관하는게 헷갈린다
	*/

	return EPageViewMode::ThirdPerson;
}
