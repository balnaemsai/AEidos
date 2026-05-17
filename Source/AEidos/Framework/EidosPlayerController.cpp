// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/EidosPlayerController.h"

#include "BlendSpaceAnalysis.h"
#include "Combat/WS_CombatDirector.h"
#include "Player/Camera/CameraModeComponent.h"
#include "Entities/Page/PageCharacter.h"
#include "Simulation/WS_SimulationOrchestrator.h"
#include "UI/GIS_UIRouter.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"

#include "World/Settlement/WS_Building.h"
#include "World/Settlement/Building/ConstructionSiteActor.h"
#include "World/Settlement/Portal/PortalActor.h"
#include "World/Settlement/TerritoryChunkActor.h"
#include "World/Settlement/WS_Population.h"
#include "World/Settlement/WS_SettlementSpace.h"
#include "Data/GIS_DataRegistry.h"
#include "Data/Definitions/BuildingDefinitionRow.h"
#include "Entities/Page/Components/StatsComponent.h"
#include "Iris/ReplicationState/PropertyNetSerializerInfoRegistry.h"

AEidosPlayerController::AEidosPlayerController()
{
	CameraMode = CreateDefaultSubobject<UCameraModeComponent>(TEXT("CameraModeComponent"));
}

void AEidosPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	EnsureValidSelectedPage();

	if (bInBuildPlacementMode)
	{
		UpdateBuildPreview();
	}
	else if (bInTerritoryPlacementMode)
	{
		UpdateTerritoryPreview();
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

	RefreshInputModeForCurrentContext();

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
	InputComponent->BindKey(EKeys::F, IE_Pressed, this, &AEidosPlayerController::OnInteractPressed);
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AEidosPlayerController::OnToggleFirstPersonUIFocus);
	InputComponent->BindKey(EKeys::LeftBracket, IE_Pressed, this, &AEidosPlayerController::OnSelectPreviousPage);
	InputComponent->BindKey(EKeys::RightBracket, IE_Pressed, this, &AEidosPlayerController::OnSelectNextPage);
	InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &AEidosPlayerController::OnEndTurnPressed);
	InputComponent->BindKey(EKeys::One, IE_Pressed, this, &AEidosPlayerController::OnCombatActionSlot1);
	InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &AEidosPlayerController::OnCombatActionSlot2);
	InputComponent->BindKey(EKeys::Three, IE_Pressed, this, &AEidosPlayerController::OnCombatActionSlot3);
	InputComponent->BindKey(EKeys::Four, IE_Pressed, this, &AEidosPlayerController::OnCombatActionSlot4);
	InputComponent->BindKey(EKeys::Five, IE_Pressed, this, &AEidosPlayerController::OnCombatActionSlot5);
	InputComponent->BindKey(EKeys::Six, IE_Pressed, this, &AEidosPlayerController::OnCombatActionSlot6);
	InputComponent->BindKey(EKeys::Seven, IE_Pressed, this, &AEidosPlayerController::OnCombatActionSlot7);
	InputComponent->BindKey(EKeys::Eight, IE_Pressed, this, &AEidosPlayerController::OnCombatActionSlot8);
	InputComponent->BindKey(EKeys::Nine, IE_Pressed, this, &AEidosPlayerController::OnCombatActionSlot9);
	InputComponent->BindKey(EKeys::Zero, IE_Pressed, this, &AEidosPlayerController::OnCombatActionSlot0);
}

void AEidosPlayerController::OnToggleView(const FInputActionValue& Value)
{
	if (CameraMode)
	{
		CameraMode->ToggleViewMode();
		if (CameraMode->GetViewMode() != EPageViewMode::FirstPerson)
		{
			bFirstPersonUIFocusMode = false;
		}
		RefreshInputModeForCurrentContext();
	}
}

void AEidosPlayerController::EnsureValidSelectedPage()
{
	if (!CameraMode)
	{
		return;
	}

	APageCharacter* CurrentPage = GetSelectedPage();
	const bool bCurrentPageValid =
		IsValid(CurrentPage)
		&& CurrentPage->IsFriendly()
		&& CurrentPage->GetStats()
		&& !CurrentPage->GetStats()->IsDead();

	if (bCurrentPageValid)
	{
		return;
	}

	UWS_Population* Population = GetWorld() ? GetWorld()->GetSubsystem<UWS_Population>() : nullptr;
	if (!Population)
	{
		return;
	}

	for (const TWeakObjectPtr<APageCharacter>& WeakPage : Population->GetOwnedPages())
	{
		APageCharacter* Candidate = WeakPage.Get();
		if (!Candidate || !Candidate->IsFriendly())
		{
			continue;
		}

		UStatsComponent* Stats = Candidate->GetStats();
		if (!Stats || Stats->IsDead())
		{
			continue;
		}

		CameraMode->SetSelectedPage(Candidate);
		CameraMode->FocusSelectedPage(true);
		RefreshInputModeForCurrentContext();

		UE_LOG(LogTemp, Log,
			TEXT("[PC] Selected page fallback to %s (PageId=%d)"),
			*GetNameSafe(Candidate),
			Candidate->GetPageEntityId());
		return;
	}
}

void AEidosPlayerController::OnToggleControlMode(const FInputActionValue& Value)
{
	if (CameraMode)
	{
		CameraMode->ToggleControlMode();
		if (CameraMode->GetControlMode() != ECameraControlMode::FollowPage)
		{
			bFirstPersonUIFocusMode = false;
		}
		RefreshInputModeForCurrentContext();
	}
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

APageCharacter* AEidosPlayerController::GetSelectedPage() const
{
	return CameraMode ? CameraMode->GetSelectedPage() : nullptr;
}

bool AEidosPlayerController::SelectPageByEntityId(int32 PageId)
{
	if (!CameraMode || PageId <= 0)
	{
		return false;
	}

	UWS_Population* Population = GetWorld() ? GetWorld()->GetSubsystem<UWS_Population>() : nullptr;
	if (!Population)
	{
		return false;
	}

	for (const TWeakObjectPtr<APageCharacter>& WeakPage : Population->GetOwnedPages())
	{
		APageCharacter* Candidate = WeakPage.Get();
		if (!Candidate || Candidate->GetPageEntityId() != PageId)
		{
			continue;
		}

		CameraMode->SetSelectedPage(Candidate);
		CameraMode->FocusSelectedPage(true);
		RefreshInputModeForCurrentContext();
		return true;
	}

	return false;
}

void AEidosPlayerController::HandleWorldSimReady()
{
	RefreshInputModeForCurrentContext();

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
		CameraMode->AddOrbitYawInput(-Axis);
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

	if (bInTerritoryPlacementMode)
	{
		ConfirmTerritoryExpansionPlacement();
		return;
	}

	if (bFirstPersonUIFocusMode)
	{
		bFirstPersonUIFocusMode = false;
		RefreshInputModeForCurrentContext();
		return;
	}

	if (UWS_CombatDirector* CombatDirector = GetWorld() ? GetWorld()->GetSubsystem<UWS_CombatDirector>() : nullptr)
	{
		if (CombatDirector->IsCombatActive())
		{
			return;
		}
	}
}

void AEidosPlayerController::OnToggleFirstPersonUIFocus()
{
	if (!CameraMode)
	{
		return;
	}

	if (CameraMode->GetControlMode() != ECameraControlMode::FollowPage ||
		CameraMode->GetViewMode() != EPageViewMode::FirstPerson)
	{
		return;
	}

	bFirstPersonUIFocusMode = !bFirstPersonUIFocusMode;
	RefreshInputModeForCurrentContext();
}

void AEidosPlayerController::ApplyHybridInputMode()
{
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	SetInputMode(InputMode);
}

void AEidosPlayerController::ApplyFirstPersonInputMode()
{
	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
	SetInputMode(FInputModeGameOnly());
}

void AEidosPlayerController::RefreshInputModeForCurrentContext()
{
	const bool bUseFirstPersonInput = IsUsingFirstPersonGameplayInput();

	if (bUseFirstPersonInput)
	{
		ApplyFirstPersonInputMode();
	}
	else
	{
		ApplyHybridInputMode();
	}
}

bool AEidosPlayerController::IsUsingFirstPersonGameplayInput() const
{
	return CameraMode
		&& !bFirstPersonUIFocusMode
		&& CameraMode->GetControlMode() == ECameraControlMode::FollowPage
		&& CameraMode->GetViewMode() == EPageViewMode::FirstPerson;
}

void AEidosPlayerController::OnInteractPressed()
{
	if (IsInPlacementMode())
	{
		return;
	}

	AActor* FocusedActor = FindFocusedInteractActor();
	if (!FocusedActor)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[PC] OnInteractPressed: no interact target in focus"));
		return;
	}

	if (!TryInteractWithActor(FocusedActor))
	{
		UE_LOG(LogTemp, Verbose, TEXT("[PC] OnInteractPressed: actor %s has no interaction handler"), *GetNameSafe(FocusedActor));
	}
}

void AEidosPlayerController::OnSelectPreviousPage()
{
	SelectAdjacentPage(-1);
}

void AEidosPlayerController::OnSelectNextPage()
{
	SelectAdjacentPage(1);
}

void AEidosPlayerController::OnEndTurnPressed()
{
	APageCharacter* SelectedPage = GetSelectedPage();
	UWS_CombatDirector* CombatDirector = GetWorld() ? GetWorld()->GetSubsystem<UWS_CombatDirector>() : nullptr;
	if (!SelectedPage || !CombatDirector)
	{
		return;
	}

	CombatDirector->RequestEndTurn(SelectedPage);
}

void AEidosPlayerController::OnCombatActionSlot1() { TriggerCombatActionSlot(0); }
void AEidosPlayerController::OnCombatActionSlot2() { TriggerCombatActionSlot(1); }
void AEidosPlayerController::OnCombatActionSlot3() { TriggerCombatActionSlot(2); }
void AEidosPlayerController::OnCombatActionSlot4() { TriggerCombatActionSlot(3); }
void AEidosPlayerController::OnCombatActionSlot5() { TriggerCombatActionSlot(4); }
void AEidosPlayerController::OnCombatActionSlot6() { TriggerCombatActionSlot(5); }
void AEidosPlayerController::OnCombatActionSlot7() { TriggerCombatActionSlot(6); }
void AEidosPlayerController::OnCombatActionSlot8() { TriggerCombatActionSlot(7); }
void AEidosPlayerController::OnCombatActionSlot9() { TriggerCombatActionSlot(8); }
void AEidosPlayerController::OnCombatActionSlot0() { TriggerCombatActionSlot(9); }

void AEidosPlayerController::TriggerCombatActionSlot(int32 SlotIndex)
{
	APageCharacter* SelectedPage = GetSelectedPage();
	UWS_CombatDirector* CombatDirector = GetWorld() ? GetWorld()->GetSubsystem<UWS_CombatDirector>() : nullptr;
	if (!SelectedPage || !CombatDirector || !CombatDirector->IsCombatActive() || !CombatDirector->IsPageTurnActive(SelectedPage))
	{
		return;
	}

	APageCharacter* TargetPage = FindFocusedHostileCombatTarget();
	CombatDirector->RequestUseCombatAction(SelectedPage, SlotIndex, TargetPage);
}

void AEidosPlayerController::BeginBuildPlacement(FName BuildingId)
{
	if (BuildingId.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] BeginBuildPlacement Failed"));
		return;
	}

	CancelTerritoryExpansionPlacement();
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

void AEidosPlayerController::BeginTerritoryExpansionPlacement()
{
	CancelBuildPlacement();
	CancelTerritoryExpansionPlacement();

	bInTerritoryPlacementMode = true;
	PendingTerritoryCoord = FIntPoint::ZeroValue;

	UE_LOG(LogTemp, Log, TEXT("[PC] BeginTerritoryExpansionPlacement"));

	SpawnOrRefreshTerritoryPreview();
	UpdateTerritoryPreview();
}

void AEidosPlayerController::CancelTerritoryExpansionPlacement()
{
	bInTerritoryPlacementMode = false;
	PendingTerritoryCoord = FIntPoint::ZeroValue;

	if (TerritoryPreviewActor.IsValid())
	{
		TerritoryPreviewActor->Destroy();
		TerritoryPreviewActor = nullptr;
	}

	UE_LOG(LogTemp, Log, TEXT("[PC] CancelTerritoryExpansionPlacement"));
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

void AEidosPlayerController::SpawnOrRefreshTerritoryPreview()
{
	if (TerritoryPreviewActor.IsValid() || !GetWorld())
	{
		return;
	}

	UWS_SettlementSpace* SettlementSpace = GetWorld()->GetSubsystem<UWS_SettlementSpace>();
	if (!SettlementSpace)
	{
		return;
	}

	TSubclassOf<ATerritoryChunkActor> PreviewClass = SettlementSpace->GetChunkActorClass();
	if (!PreviewClass)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ATerritoryChunkActor* Preview = GetWorld()->SpawnActor<ATerritoryChunkActor>(
		PreviewClass,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		Params);
	if (!Preview)
	{
		return;
	}

	Preview->InitChunk(FIntPoint::ZeroValue, SettlementSpace->GetChunkSizeCm());
	Preview->SetPreviewMode(true);
	Preview->SetPreviewValid(true);
	TerritoryPreviewActor = Preview;
}

void AEidosPlayerController::UpdateTerritoryPreview()
{
	if (!bInTerritoryPlacementMode)
	{
		return;
	}

	UWorld* World = GetWorld();
	UWS_SettlementSpace* SettlementSpace = World ? World->GetSubsystem<UWS_SettlementSpace>() : nullptr;
	if (!World || !SettlementSpace)
	{
		return;
	}

	if (!TerritoryPreviewActor.IsValid())
	{
		SpawnOrRefreshTerritoryPreview();
		if (!TerritoryPreviewActor.IsValid())
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
	Dir = Dir.GetSafeNormal();

	const TArray<FIntPoint> ExpandableChunks = SettlementSpace->GetExpandableChunks();
	if (ExpandableChunks.Num() == 0)
	{
		TerritoryPreviewActor->SetPreviewValid(false);
		return;
	}

	bool bFoundCandidate = false;
	FIntPoint BestCoord = ExpandableChunks[0];
	float BestRayDistanceSq = TNumericLimits<float>::Max();
	float BestProjectedDistance = TNumericLimits<float>::Max();

	for (const FIntPoint& CandidateCoord : ExpandableChunks)
	{
		const FVector CandidateLoc = SettlementSpace->GetChunkWorldLocation(CandidateCoord);
		const FVector ToCandidate = CandidateLoc - Start;
		const float ProjectedDistance = FVector::DotProduct(ToCandidate, Dir);
		if (ProjectedDistance <= 0.f)
		{
			continue;
		}

		const FVector ClosestPointOnRay = Start + Dir * ProjectedDistance;
		const float RayDistanceSq = FVector::DistSquared(ClosestPointOnRay, CandidateLoc);

		if (!bFoundCandidate
			|| RayDistanceSq < BestRayDistanceSq
			|| (FMath::IsNearlyEqual(RayDistanceSq, BestRayDistanceSq) && ProjectedDistance < BestProjectedDistance))
		{
			bFoundCandidate = true;
			BestCoord = CandidateCoord;
			BestRayDistanceSq = RayDistanceSq;
			BestProjectedDistance = ProjectedDistance;
		}
	}

	if (!bFoundCandidate)
	{
		float BestFallbackDistSq = TNumericLimits<float>::Max();
		for (const FIntPoint& CandidateCoord : ExpandableChunks)
		{
			const FVector CandidateLoc = SettlementSpace->GetChunkWorldLocation(CandidateCoord);
			const float DistSq = FVector::DistSquared(CandidateLoc, ControlledPawn->GetActorLocation());
			if (DistSq < BestFallbackDistSq)
			{
				BestFallbackDistSq = DistSq;
				BestCoord = CandidateCoord;
			}
		}
	}

	PendingTerritoryCoord = BestCoord;

	const FVector ChunkLoc = SettlementSpace->GetChunkWorldLocation(BestCoord);
	TerritoryPreviewActor->SetActorLocation(ChunkLoc);

	FString Reason;
	const bool bValid = SettlementSpace->CanPurchaseChunk(BestCoord, Reason);
	TerritoryPreviewActor->SetPreviewValid(bValid);
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

void AEidosPlayerController::ConfirmTerritoryExpansionPlacement()
{
	if (!bInTerritoryPlacementMode || !TerritoryPreviewActor.IsValid())
	{
		return;
	}

	UWS_SettlementSpace* SettlementSpace = GetWorld() ? GetWorld()->GetSubsystem<UWS_SettlementSpace>() : nullptr;
	if (!SettlementSpace)
	{
		return;
	}

	FString Reason;
	if (!SettlementSpace->PurchaseChunk(PendingTerritoryCoord, Reason))
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] ConfirmTerritoryExpansionPlacement denied: %s"), *Reason);
		return;
	}

	UE_LOG(LogTemp, Log,
		TEXT("[PC] Territory expansion confirmed: (%d,%d)"),
		PendingTerritoryCoord.X,
		PendingTerritoryCoord.Y);

	CancelTerritoryExpansionPlacement();
}

bool AEidosPlayerController::IsInPlacementMode() const
{
	return bInBuildPlacementMode || bInTerritoryPlacementMode;
}

FName AEidosPlayerController::GetPendingBuildingId() const
{
	return PendingBuildingId;
}

bool AEidosPlayerController::IsInTerritoryPlacementMode() const
{
	return bInTerritoryPlacementMode;
}

AActor* AEidosPlayerController::FindFocusedInteractActor() const
{
	APageCharacter* SelectedPage = GetSelectedPage();
	UWorld* World = GetWorld();
	if (!SelectedPage || !World || !PlayerCameraManager)
	{
		return nullptr;
	}

	const FVector ViewOrigin = PlayerCameraManager->GetCameraLocation();
	const FVector ViewForward = PlayerCameraManager->GetActorForwardVector().GetSafeNormal();
	const float MaxDistSq = FMath::Square(InteractMaxDistance);

	AActor* BestActor = nullptr;
	float BestScore = -FLT_MAX;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Candidate = *It;
		if (!IsValid(Candidate) || Candidate == SelectedPage)
		{
			continue;
		}

		const FVector ToTarget = Candidate->GetActorLocation() - ViewOrigin;
		const float DistSq = ToTarget.SizeSquared();
		if (DistSq > MaxDistSq || DistSq <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		const FVector ToTargetDir = ToTarget.GetSafeNormal();
		const float ForwardDot = FVector::DotProduct(ViewForward, ToTargetDir);
		if (ForwardDot < InteractForwardDotThreshold)
		{
			continue;
		}

		float InteractionPriority = -1.f;
		if (Candidate->IsA<APortalActor>())
		{
			InteractionPriority = 1000.f;
		}
		else
		{
			continue;
		}

		const float Distance = FMath::Sqrt(DistSq);
		const float Score = InteractionPriority + (ForwardDot * 100.f) - Distance;
		if (Score > BestScore)
		{
			BestScore = Score;
			BestActor = Candidate;
		}
	}

	return BestActor;
}

APageCharacter* AEidosPlayerController::FindFocusedHostileCombatTarget() const
{
	APageCharacter* SelectedPage = GetSelectedPage();
	UWorld* World = GetWorld();
	if (!SelectedPage || !World || !PlayerCameraManager)
	{
		return nullptr;
	}

	const FVector ViewOrigin = PlayerCameraManager->GetCameraLocation();
	const FVector ViewForward = PlayerCameraManager->GetActorForwardVector().GetSafeNormal();
	const float MaxDistSq = FMath::Square(InteractMaxDistance);

	APageCharacter* BestTarget = nullptr;
	float BestScore = -FLT_MAX;

	for (TActorIterator<APageCharacter> It(World); It; ++It)
	{
		APageCharacter* Candidate = *It;
		if (!Candidate || Candidate == SelectedPage || !SelectedPage->IsHostileTo(Candidate))
		{
			continue;
		}

		if (SelectedPage->IsInDungeon() != Candidate->IsInDungeon())
		{
			continue;
		}

		const FVector ToTarget = Candidate->GetActorLocation() - ViewOrigin;
		const float DistSq = ToTarget.SizeSquared();
		if (DistSq > MaxDistSq || DistSq <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		const FVector ToTargetDir = ToTarget.GetSafeNormal();
		const float ForwardDot = FVector::DotProduct(ViewForward, ToTargetDir);
		if (ForwardDot < InteractForwardDotThreshold)
		{
			continue;
		}

		const float Distance = FMath::Sqrt(DistSq);
		const float Score = (ForwardDot * 100.f) - Distance;
		if (Score > BestScore)
		{
			BestScore = Score;
			BestTarget = Candidate;
		}
	}

	return BestTarget;
}

bool AEidosPlayerController::TryInteractWithActor(AActor* TargetActor)
{
	if (!IsValid(TargetActor))
	{
		return false;
	}

	if (APortalActor* Portal = Cast<APortalActor>(TargetActor))
	{
		Portal->Interact(this);
		return true;
	}

	return false;
}

void AEidosPlayerController::SelectAdjacentPage(int32 Direction)
{
	if (Direction == 0 || !CameraMode || IsInPlacementMode())
	{
		return;
	}

	UWS_Population* Population = GetWorld() ? GetWorld()->GetSubsystem<UWS_Population>() : nullptr;
	if (!Population)
	{
		return;
	}

	TArray<APageCharacter*> Pages;
	for (const TWeakObjectPtr<APageCharacter>& WeakPage : Population->GetOwnedPages())
	{
		if (APageCharacter* Page = WeakPage.Get())
		{
			Pages.Add(Page);
		}
	}

	if (Pages.Num() == 0)
	{
		return;
	}

	Pages.Sort([](const APageCharacter& A, const APageCharacter& B)
	{
		return A.GetPageEntityId() < B.GetPageEntityId();
	});

	APageCharacter* CurrentPage = GetSelectedPage();
	int32 CurrentIndex = CurrentPage ? Pages.IndexOfByKey(CurrentPage) : INDEX_NONE;

	const int32 Step = Direction > 0 ? 1 : -1;
	const int32 StartIndex = CurrentIndex == INDEX_NONE
		? (Step > 0 ? 0 : Pages.Num() - 1)
		: (CurrentIndex + Step + Pages.Num()) % Pages.Num();

	APageCharacter* NewSelectedPage = Pages[StartIndex];
	if (!NewSelectedPage)
	{
		return;
	}

	CameraMode->SetSelectedPage(NewSelectedPage);
	CameraMode->FocusSelectedPage(true);
	RefreshInputModeForCurrentContext();

	UE_LOG(LogTemp, Log,
		TEXT("[PC] Selected page changed to %s (PageId=%d)"),
		*GetNameSafe(NewSelectedPage),
		NewSelectedPage->GetPageEntityId());
}




