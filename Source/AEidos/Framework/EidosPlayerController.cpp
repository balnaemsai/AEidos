// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/EidosPlayerController.h"

#include "BlendSpaceAnalysis.h"
#include "Player/Camera/CameraModeComponent.h"
#include "Entities/Page/PageCharacter.h"
#include "Simulation/WS_SimulationOrchestrator.h"
#include "UI/GIS_UIRouter.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "InputActionValue.h"

#include "World/Settlement/WS_Building.h"
#include "World/Settlement/Building/ConstructionSiteActor.h"
#include "Data/GIS_DataRegistry.h"
#include "Data/Definitions/BuildingDefinitionRow.h"
#include "Iris/ReplicationState/PropertyNetSerializerInfoRegistry.h"

AEidosPlayerController::AEidosPlayerController()
{
	CameraMode = CreateDefaultSubobject<UCameraModeComponent>(TEXT("CameraModeComponent"));
}

void AEidosPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bInBuildPlacementMode)
	{
		UpdateBuildPreview();
	}
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
	EI->BindAction(IA_PrimaryClick, ETriggerEvent::Started, this, &AEidosPlayerController::OnPrimaryClick);
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

	UE_LOG(LogTemp, Warning, TEXT("[PC] Pawn=%s ViewTarget=%s"), *GetNameSafe(GetPawn()), *GetNameSafe(GetViewTarget()));
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

void AEidosPlayerController::OnPrimaryClick(const FInputActionValue& Value)
{
	UE_LOG(LogTemp, Warning, TEXT("[PC] OnClick."))
	if (bInBuildPlacementMode)
	{
		ConfirmBuildPlacement();
		return;
	}
}

void AEidosPlayerController::BeginBuildPlacement(FName BuildingId)
{
	if (BuildingId.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] BeginBuildPlacement Failed"));
		return;
	}

	CancelBuildPlacement();

	bInBuildPlacementMode = true;
	PendingBuildingId = BuildingId;

	UE_LOG(LogTemp, Log, TEXT("[PC] BeginBuildPlacement: %s"), *BuildingId.ToString());

	SpawnOrRefreshBuildPreview();
	UpdateBuildPreview();
}

void AEidosPlayerController::CancelBuildPlacement()
{
	bInBuildPlacementMode = false;
	PendingBuildingId = NAME_None;

	if (BuildPreviewActor.IsValid())
	{
		BuildPreviewActor->Destroy();
		BuildPreviewActor = nullptr;
	}

	UE_LOG(LogTemp, Log, TEXT("[PC] CancelBuildPlacement"));
}

void AEidosPlayerController::SpawnOrRefreshBuildPreview()
{
	if (BuildPreviewActor.IsValid() || PendingBuildingId.IsNone())
	{
		return;
	}

	UGIS_DataRegistry* Registry = GetWorld()->GetGameInstance()->GetSubsystem<UGIS_DataRegistry>();
	if (!Registry)
	{
		return;
	}

	const FBuildingDefinitionRow* BuildingDef = Registry->GetBuildingDef(PendingBuildingId);
	if (!BuildingDef)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] SpawnBuildPreview failed: missing building def %s"), *PendingBuildingId.ToString());
		return;
	}

	TSubclassOf<AActor> PreviewClass = BuildingDef->ConstructionSiteActorClass.LoadSynchronous();
	if (!PreviewClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] SpawnBuildPreview failed: missing ConstructionSiteActorClass"));
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AConstructionSiteActor* Preview = GetWorld()->SpawnActor<AConstructionSiteActor>(PreviewClass, FVector::ZeroVector, FRotator::ZeroRotator, Params);
	if (!Preview)
	{
		return;
	}

	Preview->InitializeConstructionSite(-1, PendingBuildingId, -1);
	Preview->SetFootprint(BuildingDef->Footprint);
	Preview->SetPreviewValid(true);
	Preview->SetConstructionProgress(0.f);

	BuildPreviewActor = Preview;
}

void AEidosPlayerController::UpdateBuildPreview()
{
	if (!bInBuildPlacementMode || PendingBuildingId.IsNone())
	{
		return;
	}

	UWorld* World = GetWorld();

	if (!BuildPreviewActor.IsValid())
	{
		SpawnOrRefreshBuildPreview();
		if (!BuildPreviewActor.IsValid())
		{
			return;
		}
	}

	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	FVector Start = PlayerCameraManager ? PlayerCameraManager->GetCameraLocation() : ControlledPawn->GetActorLocation();
	FVector Dir = PlayerCameraManager ? PlayerCameraManager->GetActorForwardVector() : ControlledPawn->GetActorForwardVector();
	FVector End = Start + Dir * 1200.f;
	//UE_LOG(LogTemp, Warning, TEXT("[PC] UpdateBuilding: Loc is %f"), End.X);

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(BuildPreviewTrace), false, ControlledPawn);
	Params.AddIgnoredActor(BuildPreviewActor.Get());

	FVector PlaceLoc = Start + Dir * 400.f;
	if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		//UE_LOG(LogTemp, Warning, TEXT("[PC] UpdateBuilding: Trace Success"));
		PlaceLoc = Hit.Location;
	}

	PlaceLoc.Z = ControlledPawn->GetActorLocation().Z; //지금은 액터 높이로 하는데, 나중에는 바닥에 딱 붙여야할듯?

	FRotator PlaceRot = FRotator::ZeroRotator;

	BuildPreviewActor->SetActorLocation(PlaceLoc);
	BuildPreviewActor->SetActorRotation(PlaceRot);

	if (UWS_Building* BuildingWS = World->GetSubsystem<UWS_Building>())
	{
		FString Reason;
		const bool bValid = BuildingWS->ValidatePlacement(PendingBuildingId, PlaceLoc, PlaceRot.Yaw, Reason);
		BuildPreviewActor->SetPreviewValid(bValid);
	}
}

void AEidosPlayerController::ConfirmBuildPlacement()
{
	if (!bInBuildPlacementMode || PendingBuildingId.IsNone())
	{
		return;
	}

	UWorld* World = GetWorld();
	UWS_Building* BuildingWS = World->GetSubsystem<UWS_Building>();

	if (!BuildPreviewActor.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] ConfirmBuildPlacement failed: preview actor invalid"));
		return;
	}

	const FVector PlaceLoc = BuildPreviewActor->GetActorLocation();
	const float PlaceYaw = BuildPreviewActor->GetActorRotation().Yaw;

	FString Reason;
	if (!BuildingWS->ValidatePlacement(PendingBuildingId, PlaceLoc, PlaceYaw, Reason))
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] ConfirmBuildPlacement denied: %s"), *Reason);
		return;
	}

	const int32 RequestId = BuildingWS->RequestBuild(PendingBuildingId, PlaceLoc, PlaceYaw);
	if (RequestId == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] ConfirmBuildPlacement failed: RequestBuild returned INDEX_NONE"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[PC] Build confirmed. BuildingId=%s RequestId=%d"), *PendingBuildingId.ToString(), RequestId);

	if (BuildPreviewActor.IsValid())
	{
		BuildPreviewActor->Destroy();
		BuildPreviewActor = nullptr;
	}

	bInBuildPlacementMode = false;
	PendingBuildingId = NAME_None;
}

bool AEidosPlayerController::IsInPlacementMode() const
{
	return bInBuildPlacementMode;
}

FName AEidosPlayerController::GetPendingBuildingId() const
{
	return PendingBuildingId;
}




