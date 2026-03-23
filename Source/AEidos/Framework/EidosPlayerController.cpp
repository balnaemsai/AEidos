// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/EidosPlayerController.h"

#include "Player/Camera/CameraModeComponent.h"
#include "Entities/Page/PageCharacter.h"
#include "Simulation/WS_SimulationOrchestrator.h"
#include "UI/GIS_UIRouter.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "InputActionValue.h"

AEidosPlayerController::AEidosPlayerController()
{
	CameraMode = CreateDefaultSubobject<UCameraModeComponent>(TEXT("CameraModeComponent"));
}

void AEidosPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}


void AEidosPlayerController::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("[PC] BeginPlay: CanEver=%d Enabled=%d StartEnabled=%d"),
		PrimaryActorTick.bCanEverTick,
		IsActorTickEnabled(),
		PrimaryActorTick.bStartWithTickEnabled);

	if (UWorld* World = GetWorld())
	{
		if (auto* Orch = World->GetSubsystem<UWS_SimulationOrchestrator>())
		{
			Orch->OnWorldSimReady.AddUObject(this, &AEidosPlayerController::HandleWorldSimReady);
		}
	}

	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());

	if (ULocalPlayer* LP = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Sub = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (CommonIMC)
			{
				Sub->AddMappingContext(CommonIMC, 0);
			}
		}
	}

	{
		TArray<UCameraModeComponent*> Comps;
		GetComponents<UCameraModeComponent>(Comps);

		UE_LOG(LogTemp, Warning, TEXT("[PC] CameraModeComponent count=%d"), Comps.Num());
		for (UCameraModeComponent* C : Comps)
		{
			UE_LOG(LogTemp, Warning, TEXT("  - CamMode %s this=%p TickEnabled=%d"),
				*GetNameSafe(C), C, C ? C->IsComponentTickEnabled() : 0);
		}

		if (Comps.Num() > 0)
		{
			// Tick 켜져있는 놈을 우선 선택, 없으면 첫 번째
			UCameraModeComponent* Pick = nullptr;
			for (UCameraModeComponent* C : Comps)
			{
				if (C && C->IsComponentTickEnabled())
				{
					Pick = C;
					break;
				}
			}
			if (!Pick)
			{
				Pick = Comps[0];
			}

			// ✅ PC가 쓰는 포인터를 "실제로 존재하는 컴포넌트"로 강제 교정
			CameraMode = Pick;

			// (선택) 나머지는 비활성화해서 혼선을 없앰
			for (UCameraModeComponent* C : Comps)
			{
				if (C && C != Pick)
				{
					C->SetComponentTickEnabled(false);
					C->Deactivate();
				}
			}

			UE_LOG(LogTemp, Warning, TEXT("[PC] CameraMode picked: %s this=%p"),
				*GetNameSafe(CameraMode), CameraMode);
		}
	}
}

void AEidosPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EI = CastChecked<UEnhancedInputComponent>(InputComponent);
	EI->BindAction(ToggleViewAction, ETriggerEvent::Started, this, &AEidosPlayerController::OnToggleView);
	EI->BindAction(ToggleControlModeAction, ETriggerEvent::Started, this, &AEidosPlayerController::OnToggleControlMode);
	EI->BindAction(LookAction, ETriggerEvent::Triggered, this, &AEidosPlayerController::OnLook);
	EI->BindAction(IA_OrbitYaw, ETriggerEvent::Triggered, this, &AEidosPlayerController::OnOrbitYaw);
	EI->BindAction(IA_Zoom,     ETriggerEvent::Triggered, this, &AEidosPlayerController::OnZoom);
}

void AEidosPlayerController::OnToggleView(const FInputActionValue& Value)
{
	if (CameraMode) CameraMode->ToggleViewMode();
}

void AEidosPlayerController::OnToggleControlMode(const FInputActionValue& Value)
{
	if (CameraMode) CameraMode->ToggleControlMode();
}

APageCharacter* AEidosPlayerController::FindAnyPage() const
{
	if (UWorld* W = GetWorld())
	{
		for (TActorIterator<APageCharacter> It(W); It; ++It)
		{
			return *It;
		}
	}
	return nullptr;
}

void AEidosPlayerController::HandleWorldSimReady()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (auto* UIRouter = GI->GetSubsystem<UGIS_UIRouter>())
		{
			UIRouter->RequestUIState(EUIState::InGame);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[PC] WorldSimReady. Selecting control page..."));

	if (!CameraMode)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] CameraMode is null."));
		return;
	}

	CameraMode->InitializeForController(this);

	if (APageCharacter* Any = FindAnyPage())
	{
		CameraMode->SetSelectedPage(Any);
		UE_LOG(LogTemp, Log, TEXT("[PC] Control Page = %s"), *Any->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] Still no PageCharacter on WorldSimReady."));
	}

	UE_LOG(LogTemp, Warning, TEXT("[PC] Pawn=%s ViewTarget=%s"),
	*GetNameSafe(GetPawn()),
	*GetNameSafe(GetViewTarget()));
	UE_LOG(LogTemp, Warning, TEXT("[PC] SelectedPage Is %s"), *GetNameSafe(CameraMode->GetSelectedPage()));
}

void AEidosPlayerController::OnLook(const FInputActionValue& Value)
{
	const FVector2D V = Value.Get<FVector2D>();
	if (V.IsNearlyZero())
	{
		return;
	}

	if (!IsValid(CameraMode))
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] OnLook aborted: CameraMode invalid"));
		return;
	}

	const EPageViewMode ViewMode = CameraMode->GetViewMode();

	if (ViewMode == EPageViewMode::FirstPerson)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] OnLook Working"));
		AddYawInput(V.X);
		AddPitchInput(V.Y);
	}
}

void AEidosPlayerController::OnOrbitYaw(const FInputActionValue& Value)
{
	const float Axis = Value.Get<float>();
	if (FMath::IsNearlyZero(Axis)) return;

	if (CameraMode)
	{
		//UE_LOG(LogTemp, Warning, TEXT("[PC] AddOrbit."));
		CameraMode->AddOrbitYawInput(Axis);
	}
}

void AEidosPlayerController::OnZoom(const FInputActionValue& Value)
{
	const float Axis = Value.Get<float>();
	if (FMath::IsNearlyZero(Axis)) return;

	if (CameraMode)
	{
		//UE_LOG(LogTemp, Warning, TEXT("[PC] OnZoom."));
		CameraMode->AddZoomInput(Axis);
	}
}