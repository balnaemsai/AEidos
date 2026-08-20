// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/EidosPlayerController.h"

#include "BlendSpaceAnalysis.h"
#include "Combat/WS_CombatDirector.h"
#include "Player/Camera/CameraModeComponent.h"
#include "Entities/Page/PageCharacter.h"
#include "Entities/Items/EquipmentComponent.h"
#include "Entities/Items/InventoryComponent.h"
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
#include "World/Dungeon/DungeonCoreActor.h"
#include "World/Interaction/WorldInteractionInterface.h"
#include "World/Interaction/WorldBlockActor.h"
#include "UI/HUD/WorldInteraction/WorldInteractionRadialWidget.h"
#include "UI/HUD/WorldInteraction/WorldInteractionFocusWidget.h"
#include "World/Dungeon/DungeonReturnPortalActor.h"
#include "World/Settlement/Portal/PortalActor.h"
#include "World/Settlement/TerritoryChunkActor.h"
#include "World/Settlement/WS_Population.h"
#include "World/Settlement/WS_SettlementSpace.h"
#include "Data/GIS_DataRegistry.h"
#include "Data/Definitions/BuildingDefinitionRow.h"
#include "Data/Definitions/ItemDefinitionRow.h"
#include "Data/Definitions/SkillDefinitionRow.h"
#include "Entities/Page/Components/StatsComponent.h"
#include "Iris/ReplicationState/PropertyNetSerializerInfoRegistry.h"
#include "Save/GIS_SaveLoad.h"
#include "Engine/Engine.h"

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
	else if (bInBlockPlacementMode)
	{
		UpdateBlockPreview();
	}
	else
	{
		UpdateWorldInteractionFocus();
	}
}


void AEidosPlayerController::BeginPlay()
{
	Super::BeginPlay();

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

		if (Comps.Num() > 0)
		{
			// Tick 耳쒖졇?덈뒗 ?덉쓣 ?곗꽑 ?좏깮, ?놁쑝硫?泥?踰덉㎏
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

			// ??PC媛 ?곕뒗 ?ъ씤?곕? "?ㅼ젣濡?議댁옱?섎뒗 而댄룷?뚰듃"濡?媛뺤젣 援먯젙
			CameraMode = Pick;
			
			for (UCameraModeComponent* C : Comps)
			{
				if (C && C != Pick)
				{
					C->SetComponentTickEnabled(false);
					C->Deactivate();
				}
			}

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
	EI->BindAction(IA_PrimaryClick, ETriggerEvent::Triggered, this, &AEidosPlayerController::OnPrimaryClickHeld);
	EI->BindAction(IA_PrimaryClick, ETriggerEvent::Completed, this, &AEidosPlayerController::OnPrimaryClickCompleted);
	EI->BindAction(IA_PrimaryClick, ETriggerEvent::Canceled, this, &AEidosPlayerController::OnPrimaryClickCompleted);
	InputComponent->BindKey(EKeys::F, IE_Pressed, this, &AEidosPlayerController::OnInteractPressed);
	InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AEidosPlayerController::OnSecondaryClick);
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AEidosPlayerController::OnToggleFirstPersonUIFocus);
	InputComponent->BindKey(EKeys::LeftBracket, IE_Pressed, this, &AEidosPlayerController::OnSelectPreviousPage);
	InputComponent->BindKey(EKeys::RightBracket, IE_Pressed, this, &AEidosPlayerController::OnSelectNextPage);
	InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &AEidosPlayerController::OnEndTurnPressed);
	InputComponent->BindKey(EKeys::F6, IE_Pressed, this, &AEidosPlayerController::OnQuickSavePressed);
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
		if (!Stats || Stats->IsDead() || Stats->IsDowned() || Stats->IsRecovering())
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
	if (ActiveWorldInteractionRadial && ActiveWorldInteractionRadial->IsInViewport())
	{
		return;
	}
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

	if (bInBlockPlacementMode)
	{
		ConfirmBlockPlacement();
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
			APageCharacter* SelectedPage = GetSelectedPage();
			if (SelectedPage && CombatDirector->IsPageTurnActive(SelectedPage))
			{
				AActor* FocusedTarget = FindFocusedCombatActionTarget();
				if (!SelectCombatTarget(FocusedTarget))
				{
					SetCombatTargetingHint(FText::FromString(TEXT("NO VALID HOSTILE TARGET IN FOCUS")));
					return;
				}

				if (PendingCombatActionSlot != INDEX_NONE)
				{
					ExecutePendingCombatAction(SelectedPage, CombatDirector);
				}
				else
				{
					SetCombatTargetingHint(FText::FromString(TEXT("TARGET SELECTED - CHOOSE AN ACTION")));
				}
			}
			return;
		}
	}

	if (!SelectedWorldInteractionId.IsNone())
	{
		if (ExecuteSelectedWorldInteraction() && GetWorld())
		{
			LastWorldInteractionTime = GetWorld()->GetTimeSeconds();
		}
		return;
	}

	if (AActor* FocusedActor = FindFocusedWorldInteractionActor())
	{
		if (ExecuteDefaultWorldInteraction(FocusedActor) && GetWorld())
		{
			LastWorldInteractionTime = GetWorld()->GetTimeSeconds();
		}
	}
}

void AEidosPlayerController::OnQuickSavePressed()
{
	UWorld* World = GetWorld();
	UGameInstance* GameInstance = GetGameInstance();
	UGIS_SaveLoad* SaveLoad = GameInstance ? GameInstance->GetSubsystem<UGIS_SaveLoad>() : nullptr;
	if (!World || !SaveLoad)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Save] Quick save failed: world or save subsystem is unavailable."));
		return;
	}

	FString FailureReason;
	if (!SaveLoad->CanSaveWorld(*World, FailureReason))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Save] Quick save unavailable: %s"), *FailureReason);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 4.f, FColor::Yellow,
				FString::Printf(TEXT("Save unavailable: %s"), *FailureReason));
		}
		return;
	}

	constexpr const TCHAR* QuickSaveSlotName = TEXT("Slot0");
	const bool bSaved = SaveLoad->SaveToSlot(*World, QuickSaveSlotName, 0);
	if (bSaved)
	{
		UE_LOG(LogTemp, Log, TEXT("[Save] Quick save completed (Slot=%s)"), QuickSaveSlotName);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Save] Quick save failed (Slot=%s)"), QuickSaveSlotName);
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, bSaved ? FColor::Green : FColor::Red,
			bSaved ? TEXT("Game saved.") : TEXT("Game save failed."));
	}
}

void AEidosPlayerController::OnPrimaryClickHeld(const FInputActionValue& Value)
{
	// Placement and combat consume primary input differently. Repeating those actions while held
	// would cause duplicate placement or combat target commands, so only repeat world interactions.
	if (IsInPlacementMode() || bFirstPersonUIFocusMode || bSuppressWorldInteractionRepeatUntilPrimaryReleased || !GetWorld())
	{
		return;
	}
	if (ActiveWorldInteractionRadial && ActiveWorldInteractionRadial->IsInViewport())
	{
		return;
	}

	if (const UWS_CombatDirector* CombatDirector = GetWorld()->GetSubsystem<UWS_CombatDirector>(); CombatDirector && CombatDirector->IsCombatActive())
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastWorldInteractionTime < WorldInteractionRepeatInterval)
	{
		return;
	}

	const bool bExecuted = !SelectedWorldInteractionId.IsNone()
		? ExecuteSelectedWorldInteraction()
		: ExecuteDefaultWorldInteraction(FindFocusedWorldInteractionActor());
	if (bExecuted)
	{
		LastWorldInteractionTime = Now;
	}
}

void AEidosPlayerController::OnPrimaryClickCompleted(const FInputActionValue& Value)
{
	bSuppressWorldInteractionRepeatUntilPrimaryReleased = false;
}

void AEidosPlayerController::OnSecondaryClick()
{
	if (bInBlockPlacementMode)
	{
		CancelBlockPlacement();
		return;
	}

	if (IsInPlacementMode() || bFirstPersonUIFocusMode || !GetWorld())
	{
		return;
	}

	if (AActor* FocusedActor = FindFocusedWorldInteractionActor())
	{
		OpenWorldInteractionRadial(FocusedActor);
	}
}

void AEidosPlayerController::OnToggleFirstPersonUIFocus()
{
	if (ActiveWorldInteractionRadial && ActiveWorldInteractionRadial->IsInViewport())
	{
		CloseWorldInteractionRadial();
		return;
	}

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
	RequestCombatEndTurn();
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

bool AEidosPlayerController::UseCombatActionSlot(int32 SlotIndex)
{
	return TriggerCombatActionSlot(SlotIndex);
}

bool AEidosPlayerController::RequestCombatEndTurn()
{
	APageCharacter* SelectedPage = GetSelectedPage();
	UWS_CombatDirector* CombatDirector = GetWorld() ? GetWorld()->GetSubsystem<UWS_CombatDirector>() : nullptr;
	const bool bAccepted = SelectedPage && CombatDirector && CombatDirector->RequestEndTurn(SelectedPage);
	if (bAccepted)
	{
		PendingCombatActionSlot = INDEX_NONE;
		SetCombatTargetingHint(FText::FromString(TEXT("TURN ENDED")));
	}
	return bAccepted;
}

bool AEidosPlayerController::TriggerCombatActionSlot(int32 SlotIndex)
{
	APageCharacter* SelectedPage = GetSelectedPage();
	UWS_CombatDirector* CombatDirector = GetWorld() ? GetWorld()->GetSubsystem<UWS_CombatDirector>() : nullptr;
	if (!SelectedPage || !CombatDirector || !CombatDirector->IsCombatActive() || !CombatDirector->IsPageTurnActive(SelectedPage))
	{
		return false;
	}

	FPageCombatActionSlot ActionSlot;
	if (!SelectedPage->GetCombatActionSlot(SlotIndex, ActionSlot) || ActionSlot.ActionType == EPageCombatActionType::None)
	{
		SetCombatTargetingHint(FText::FromString(TEXT("THIS QUICKBAR SLOT IS EMPTY")));
		return false;
	}

	if (ActionSlot.ActionType == EPageCombatActionType::EndTurn)
	{
		return RequestCombatEndTurn();
	}

	if (ActionSlot.ActionType != EPageCombatActionType::ActiveSkill)
	{
		SetCombatTargetingHint(FText::FromString(TEXT("THIS ACTION IS NOT IMPLEMENTED")));
		return false;
	}

	UGameInstance* GI = GetGameInstance();
	UGIS_DataRegistry* Registry = GI ? GI->GetSubsystem<UGIS_DataRegistry>() : nullptr;
	const FSkillDefinitionRow* SkillDef = Registry && Registry->EnsureReadySync() ? Registry->GetSkillDef(ActionSlot.ActionId) : nullptr;
	if (!SkillDef || !SkillDef->bIsActiveCombatSkill)
	{
		SetCombatTargetingHint(FText::FromString(TEXT("ACTION DATA IS UNAVAILABLE")));
		return false;
	}

	if (!SkillDef->bRequiresTarget)
	{
		return CombatDirector->RequestUseCombatAction(SelectedPage, SlotIndex, nullptr);
	}

	PendingCombatActionSlot = SlotIndex;
	SetCombatTargetingHint(FText::Format(FText::FromString(TEXT("{0}: SELECT A HOSTILE TARGET")),
		ActionSlot.DisplayName.IsEmpty() ? FText::FromName(ActionSlot.ActionId) : ActionSlot.DisplayName));
	return true;
}

bool AEidosPlayerController::ExecutePendingCombatAction(APageCharacter* SelectedPage, UWS_CombatDirector* CombatDirector)
{
	if (!SelectedPage || !CombatDirector || PendingCombatActionSlot == INDEX_NONE)
	{
		return false;
	}

	FPageCombatActionSlot ActionSlot;
	AActor* TargetActor = SelectedCombatTarget.Get();
	if (!SelectedPage->GetCombatActionSlot(PendingCombatActionSlot, ActionSlot) || !IsValid(TargetActor))
	{
		SetCombatTargetingHint(FText::FromString(TEXT("SELECT A VALID TARGET")));
		return false;
	}

	UGameInstance* GI = GetGameInstance();
	UGIS_DataRegistry* Registry = GI ? GI->GetSubsystem<UGIS_DataRegistry>() : nullptr;
	const FSkillDefinitionRow* SkillDef = Registry && Registry->EnsureReadySync() ? Registry->GetSkillDef(ActionSlot.ActionId) : nullptr;
	if (!SkillDef)
	{
		SetCombatTargetingHint(FText::FromString(TEXT("ACTION DATA IS UNAVAILABLE")));
		return false;
	}

	const float Distance = FVector::Distance(SelectedPage->GetActorLocation(), TargetActor->GetActorLocation());
	if (Distance > SkillDef->CombatRangeCm)
	{
		SetCombatTargetingHint(FText::Format(FText::FromString(TEXT("TARGET OUT OF RANGE ({0}/{1})")),
			FMath::RoundToInt(Distance), FMath::RoundToInt(SkillDef->CombatRangeCm)));
		return false;
	}
	if (CombatDirector->GetActionPointsRemaining(SelectedPage) < SkillDef->CombatActionPointCost)
	{
		SetCombatTargetingHint(FText::FromString(TEXT("NOT ENOUGH AP")));
		return false;
	}

	const int32 SlotToExecute = PendingCombatActionSlot;
	if (CombatDirector->RequestUseCombatAction(SelectedPage, SlotToExecute, TargetActor))
	{
		PendingCombatActionSlot = INDEX_NONE;
		SetCombatTargetingHint(FText::Format(FText::FromString(TEXT("{0} EXECUTED")), ActionSlot.DisplayName));
		return true;
	}

	SetCombatTargetingHint(FText::FromString(TEXT("ACTION COULD NOT BE EXECUTED")));
	return false;
}

void AEidosPlayerController::SetCombatTargetingHint(const FText& NewHint)
{
	CombatTargetingHint = NewHint;
}

void AEidosPlayerController::SetSelectedCombatTarget(AActor* NewTarget)
{
	if (SelectedCombatTarget.Get() == NewTarget)
	{
		return;
	}

	SelectedCombatTarget = NewTarget;
	OnSelectedCombatTargetChanged(NewTarget);
	UE_LOG(LogTemp, Log, TEXT("[Combat] Selected target: %s"), *GetNameSafe(NewTarget));
}

void AEidosPlayerController::ClearSelectedCombatTarget()
{
	SetSelectedCombatTarget(nullptr);
}

bool AEidosPlayerController::SelectCombatTarget(AActor* TargetActor)
{
	APageCharacter* SelectedPage = GetSelectedPage();
	if (!SelectedPage || !IsValid(TargetActor))
	{
		return false;
	}

	if (APageCharacter* TargetPage = Cast<APageCharacter>(TargetActor))
	{
		if (!SelectedPage->IsHostileTo(TargetPage) || SelectedPage->IsInDungeon() != TargetPage->IsInDungeon())
		{
			return false;
		}
	}
	else if (Cast<ADungeonCoreActor>(TargetActor))
	{
		if (!SelectedPage->IsInDungeon())
		{
			return false;
		}
	}
	else
	{
		return false;
	}

	SetSelectedCombatTarget(TargetActor);
	return true;
}

void AEidosPlayerController::BeginBuildPlacement(FName BuildingId)
{
	if (BuildingId.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] BeginBuildPlacement Failed"));
		return;
	}

	CancelTerritoryExpansionPlacement();
	CancelBlockPlacement();
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
	CancelBlockPlacement();
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

	PlaceLoc.Z = ControlledPawn->GetActorLocation().Z; //吏湲덉? ?≫꽣 ?믪씠濡??섎뒗?? ?섏쨷?먮뒗 諛붾떏????遺숈뿬?쇳븷??

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
	return bInBuildPlacementMode || bInTerritoryPlacementMode || bInBlockPlacementMode;
}

FName AEidosPlayerController::GetPendingBuildingId() const
{
	return PendingBuildingId;
}

bool AEidosPlayerController::IsInTerritoryPlacementMode() const
{
	return bInTerritoryPlacementMode;
}

bool AEidosPlayerController::BeginBlockPlacement(FName ItemId)
{
	APageCharacter* SelectedPage = GetSelectedPage();
	UInventoryComponent* Inventory = SelectedPage ? SelectedPage->GetInventory() : nullptr;
	UGIS_DataRegistry* Registry = GetWorld() && GetWorld()->GetGameInstance()
		? GetWorld()->GetGameInstance()->GetSubsystem<UGIS_DataRegistry>() : nullptr;
	const bool bHasItem = Inventory && Inventory->GetStacks().ContainsByPredicate(
		[ItemId](const FItemStack& Stack) { return Stack.ItemId == ItemId && Stack.Quantity > 0; });
	const FItemDefinitionRow* ItemDef = Registry && Registry->EnsureReadySync() ? Registry->GetItemDef(ItemId) : nullptr;
	if (!bHasItem || !ItemDef || ItemDef->PlacedBlockClass.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BlockPlacement] Cannot place Item=%s. It is missing from inventory or has no PlacedBlockClass."), *ItemId.ToString());
		return false;
	}

	CancelBuildPlacement();
	CancelTerritoryExpansionPlacement();
	CancelBlockPlacement();
	bInBlockPlacementMode = true;
	PendingBlockItemId = ItemId;
	SpawnOrRefreshBlockPreview();
	UpdateBlockPreview();
	return BlockPreviewActor.IsValid();
}

void AEidosPlayerController::CancelBlockPlacement()
{
	bInBlockPlacementMode = false;
	bBlockPlacementPreviewValid = false;
	PendingBlockItemId = NAME_None;
	if (BlockPreviewActor.IsValid())
	{
		BlockPreviewActor->Destroy();
		BlockPreviewActor = nullptr;
	}
}

void AEidosPlayerController::SpawnOrRefreshBlockPreview()
{
	if (BlockPreviewActor.IsValid() || PendingBlockItemId.IsNone() || !GetWorld())
	{
		return;
	}
	UGIS_DataRegistry* Registry = GetWorld()->GetGameInstance()
		? GetWorld()->GetGameInstance()->GetSubsystem<UGIS_DataRegistry>() : nullptr;
	const FItemDefinitionRow* ItemDef = Registry && Registry->EnsureReadySync()
		? Registry->GetItemDef(PendingBlockItemId) : nullptr;
	TSubclassOf<AWorldBlockActor> BlockClass = ItemDef ? ItemDef->PlacedBlockClass.LoadSynchronous() : nullptr;
	if (!BlockClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BlockPlacement] Missing PlacedBlockClass for Item=%s"), *PendingBlockItemId.ToString());
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AWorldBlockActor* Preview = GetWorld()->SpawnActor<AWorldBlockActor>(BlockClass, FVector::ZeroVector, FRotator::ZeroRotator, Params))
	{
		Preview->SetPlacementPreview(true, false);
		BlockPreviewActor = Preview;
	}
}

void AEidosPlayerController::UpdateBlockPreview()
{
	if (!bInBlockPlacementMode || PendingBlockItemId.IsNone() || !GetWorld())
	{
		return;
	}
	if (!BlockPreviewActor.IsValid())
	{
		SpawnOrRefreshBlockPreview();
		if (!BlockPreviewActor.IsValid()) return;
	}

	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn) return;
	const FVector Start = PlayerCameraManager ? PlayerCameraManager->GetCameraLocation() : ControlledPawn->GetActorLocation();
	const FVector Direction = (PlayerCameraManager ? PlayerCameraManager->GetActorForwardVector() : ControlledPawn->GetActorForwardVector()).GetSafeNormal();
	FHitResult Hit;
	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(BlockPlacementTrace), false, ControlledPawn);
	TraceParams.AddIgnoredActor(BlockPreviewActor.Get());
	const bool bHasSurface = GetWorld()->LineTraceSingleByChannel(Hit, Start, Start + Direction * BlockPlacementMaxDistance, ECC_Visibility, TraceParams);
	if (!bHasSurface)
	{
		bBlockPlacementPreviewValid = false;
		BlockPreviewActor->SetPlacementPreview(true, false);
		return;
	}

	const FVector Normal = Hit.ImpactNormal.GetSafeNormal();
	const FVector Extent = BlockPreviewActor->GetPlacementBoundsExtent();
	const float SurfaceOffset = FVector::DotProduct(Extent, Normal.GetAbs()) + 2.f;
	const FVector PlaceLocation = Hit.ImpactPoint + Normal * SurfaceOffset;
	BlockPreviewActor->SetActorLocation(PlaceLocation);
	BlockPreviewActor->SetActorRotation(FRotator::ZeroRotator);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	FCollisionQueryParams OverlapParams(SCENE_QUERY_STAT(BlockPlacementOverlap), false, ControlledPawn);
	OverlapParams.AddIgnoredActor(BlockPreviewActor.Get());
	const bool bBlocked = GetWorld()->OverlapAnyTestByObjectType(
		PlaceLocation, FQuat::Identity, ObjectParams, FCollisionShape::MakeBox(Extent * 0.95f), OverlapParams);
	bBlockPlacementPreviewValid = !bBlocked;
	BlockPreviewActor->SetPlacementPreview(true, bBlockPlacementPreviewValid);
}

void AEidosPlayerController::ConfirmBlockPlacement()
{
	if (!bInBlockPlacementMode || !bBlockPlacementPreviewValid || !BlockPreviewActor.IsValid())
	{
		return;
	}
	APageCharacter* SelectedPage = GetSelectedPage();
	UInventoryComponent* Inventory = SelectedPage ? SelectedPage->GetInventory() : nullptr;
	UGIS_DataRegistry* Registry = GetWorld() && GetWorld()->GetGameInstance()
		? GetWorld()->GetGameInstance()->GetSubsystem<UGIS_DataRegistry>() : nullptr;
	const FItemDefinitionRow* ItemDef = Registry && Registry->EnsureReadySync()
		? Registry->GetItemDef(PendingBlockItemId) : nullptr;
	TSubclassOf<AWorldBlockActor> BlockClass = ItemDef ? ItemDef->PlacedBlockClass.LoadSynchronous() : nullptr;
	if (!Inventory || !BlockClass)
	{
		CancelBlockPlacement();
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AWorldBlockActor* PlacedBlock = GetWorld()->SpawnActor<AWorldBlockActor>(BlockClass,
		BlockPreviewActor->GetActorLocation(), BlockPreviewActor->GetActorRotation(), Params);
	if (!PlacedBlock)
	{
		return;
	}

	float IgnoredQuality = 0.f;
	if (Inventory->TryRemoveItem(PendingBlockItemId, 1, IgnoredQuality) != 1)
	{
		PlacedBlock->Destroy();
		CancelBlockPlacement();
		return;
	}
	PlacedBlock->SetPlacementPreview(false);
	UE_LOG(LogTemp, Log, TEXT("[BlockPlacement] Placed Item=%s at %s"), *PendingBlockItemId.ToString(), *PlacedBlock->GetActorLocation().ToString());

	const bool bHasMore = Inventory->GetStacks().ContainsByPredicate([this](const FItemStack& Stack)
	{
		return Stack.ItemId == PendingBlockItemId && Stack.Quantity > 0;
	});
	if (!bHasMore)
	{
		CancelBlockPlacement();
	}
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
	const float MaxDistSq = FMath::Square(CombatTargetMaxDistance);

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
		if (const APageCharacter* CandidatePage = Cast<APageCharacter>(Candidate); CandidatePage
			&& CandidatePage->IsFriendly()
			&& CandidatePage->GetStats()
			&& CandidatePage->GetStats()->IsDowned()
			&& CandidatePage->IsInDungeon() == SelectedPage->IsInDungeon())
		{
			InteractionPriority = 1100.f;
		}
		else if (Candidate->IsA<APortalActor>())
		{
			InteractionPriority = 1000.f;
		}
	else if (Candidate->IsA<ADungeonCoreActor>())
	{
		InteractionPriority = 950.f;
	}
	else if (Candidate->IsA<ADungeonReturnPortalActor>())
	{
		InteractionPriority = 975.f;
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

AActor* AEidosPlayerController::FindFocusedCombatActionTarget() const
{
	APageCharacter* SelectedPage = GetSelectedPage();
	UWorld* World = GetWorld();
	if (!SelectedPage || !World || !PlayerCameraManager)
	{
		return nullptr;
	}

	const FVector ViewOrigin = PlayerCameraManager->GetCameraLocation();
	const FVector ViewForward = PlayerCameraManager->GetActorForwardVector().GetSafeNormal();
	const float MaxDistSq = FMath::Square(CombatTargetMaxDistance);

	AActor* BestTarget = nullptr;
	float BestScore = -FLT_MAX;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* CandidateActor = *It;
		if (!IsValid(CandidateActor) || CandidateActor == SelectedPage)
		{
			continue;
		}

		APageCharacter* CandidatePage = Cast<APageCharacter>(CandidateActor);
		ADungeonCoreActor* CandidateCore = Cast<ADungeonCoreActor>(CandidateActor);
		if (CandidatePage)
		{
			if (!SelectedPage->IsHostileTo(CandidatePage) || SelectedPage->IsInDungeon() != CandidatePage->IsInDungeon())
			{
				continue;
			}
		}
		else if (CandidateCore)
		{
			if (!SelectedPage->IsInDungeon())
			{
				continue;
			}
		}
		else
		{
			continue;
		}

		const FVector ToTarget = CandidateActor->GetActorLocation() - ViewOrigin;
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
			BestTarget = CandidateActor;
		}
	}

	return BestTarget;
}

AActor* AEidosPlayerController::FindFocusedWorldInteractionActor() const
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
		if (!IsValid(Candidate) || Candidate == SelectedPage
			|| !Candidate->GetClass()->ImplementsInterface(UWorldInteractionInterface::StaticClass()))
		{
			continue;
		}

		const FVector ToTarget = Candidate->GetActorLocation() - ViewOrigin;
		const float DistSq = ToTarget.SizeSquared();
		if (DistSq > MaxDistSq || DistSq <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		const float ForwardDot = FVector::DotProduct(ViewForward, ToTarget.GetSafeNormal());
		if (ForwardDot < InteractForwardDotThreshold)
		{
			continue;
		}

		const float Score = (ForwardDot * 100.f) - FMath::Sqrt(DistSq);
		if (Score > BestScore)
		{
			BestScore = Score;
			BestActor = Candidate;
		}
	}

	return BestActor;
}

bool AEidosPlayerController::ExecuteDefaultWorldInteraction(AActor* TargetActor)
{
	TArray<FWorldInteractionOption> Options;
	FWorldInteractionOption PreparedOption;
	APageCharacter* SelectedPage = GetSelectedPage();
	if (!SelectedPage || (SelectedPage->GetStats() && (SelectedPage->GetStats()->IsDead() || SelectedPage->GetStats()->IsDowned() || SelectedPage->GetStats()->IsRecovering()))
		|| !ResolvePreparedWorldInteraction(TargetActor, Options, PreparedOption))
	{
		return false;
	}

	SelectedPage->BeginManualWorkOverride();
	return IWorldInteractionInterface::Execute_ExecuteWorldInteraction(TargetActor, SelectedPage, PreparedOption.InteractionId);
}

bool AEidosPlayerController::ResolvePreparedWorldInteraction(AActor* TargetActor,
	TArray<FWorldInteractionOption>& OutOptions, FWorldInteractionOption& OutPreparedOption) const
{
	OutOptions.Reset();
	OutPreparedOption = FWorldInteractionOption{};
	APageCharacter* SelectedPage = GetSelectedPage();
	if (!SelectedPage || (SelectedPage->GetStats() && (SelectedPage->GetStats()->IsDead() || SelectedPage->GetStats()->IsDowned() || SelectedPage->GetStats()->IsRecovering())) || !IsValid(TargetActor)
		|| !TargetActor->GetClass()->ImplementsInterface(UWorldInteractionInterface::StaticClass()))
	{
		return false;
	}

	IWorldInteractionInterface::Execute_GetAvailableWorldInteractions(TargetActor, SelectedPage, OutOptions);
	if (OutOptions.IsEmpty()) return false;

	// A radial selection follows its Page, not the block that was originally clicked.
	// If the focused block offers the selected action, keep it armed; otherwise the
	// normal per-block default resolution below provides a safe fallback.
	if (SelectedWorldInteractionPage.Get() == SelectedPage && !SelectedWorldInteractionId.IsNone())
	{
		if (const FWorldInteractionOption* SelectedOption = OutOptions.FindByPredicate(
			[this](const FWorldInteractionOption& Option)
			{
				return Option.InteractionId == SelectedWorldInteractionId;
			}))
		{
			OutPreparedOption = *SelectedOption;
			return true;
		}
	}

	TArray<const FWorldInteractionOption*> Candidates;
	for (const FWorldInteractionOption& Option : OutOptions)
	{
		if (Option.bIsDefault) Candidates.Add(&Option);
	}
	if (Candidates.IsEmpty())
	{
		for (const FWorldInteractionOption& Option : OutOptions) Candidates.Add(&Option);
	}

	const UEquipmentComponent* Equipment = SelectedPage->GetEquipment();
	if (Equipment)
	{
		for (const EPageEquipmentSlot HandSlot : { EPageEquipmentSlot::RightHand, EPageEquipmentSlot::LeftHand })
		{
			for (const FWorldInteractionOption* Option : Candidates)
			{
				if (Option && !Option->RequiredToolTag.IsNone()
					&& Equipment->HasToolTagInSlot(HandSlot, Option->RequiredToolTag))
				{
					OutPreparedOption = *Option;
					return true;
				}
			}
		}
	}

	for (const FWorldInteractionOption* Option : Candidates)
	{
		if (Option && Option->RequiredToolTag.IsNone())
		{
			OutPreparedOption = *Option;
			return true;
		}
	}

	if (Equipment)
	{
		for (const FWorldInteractionOption* Option : Candidates)
		{
			if (Option && Equipment->CanUseToolForInteraction(Option->RequiredToolTag))
			{
				OutPreparedOption = *Option;
				return true;
			}
		}
	}
	return false;
}

FText AEidosPlayerController::FormatPreparedWorldInteraction(const FWorldInteractionOption& Option) const
{
	const FText ActionName = Option.DisplayName.IsEmpty() ? FText::FromName(Option.InteractionId) : Option.DisplayName;
	if (Option.RequiredToolTag.IsNone()) return ActionName;

	FString ToolName = Option.RequiredToolTag.ToString();
	FString Prefix;
	if (ToolName.Split(TEXT("."), &Prefix, &ToolName, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
	{
		// Use the human-readable final tag segment: Tool.Pickaxe becomes Pickaxe.
	}
	return FText::Format(FText::FromString(TEXT("{0} ({1})")), ActionName, FText::FromString(ToolName));
}

void AEidosPlayerController::UpdateWorldInteractionFocus()
{
	if (bFirstPersonUIFocusMode || (ActiveWorldInteractionRadial && ActiveWorldInteractionRadial->IsInViewport()))
	{
		ClearWorldInteractionFocus();
		return;
	}

	AActor* TargetActor = FindFocusedWorldInteractionActor();
	TArray<FWorldInteractionOption> Options;
	FWorldInteractionOption PreparedOption;
	bool bHasPreparedAction = ResolvePreparedWorldInteraction(TargetActor, Options, PreparedOption);
	AWorldBlockActor* FocusedBlock = Cast<AWorldBlockActor>(TargetActor);
	if (!FocusedBlock || Options.IsEmpty())
	{
		ClearWorldInteractionFocus();
		return;
	}

	if (FocusedWorldInteractionActor.Get() != FocusedBlock)
	{
		ClearWorldInteractionFocus();
		FocusedWorldInteractionActor = FocusedBlock;
		FocusedBlock->SetInteractionFocused(true);
	}

	if (!WorldInteractionFocusClass) return;
	if (!ActiveWorldInteractionFocus)
	{
		ActiveWorldInteractionFocus = CreateWidget<UWorldInteractionFocusWidget>(this, WorldInteractionFocusClass);
		if (!ActiveWorldInteractionFocus) return;
		ActiveWorldInteractionFocus->AddToViewport(90);
		ActiveWorldInteractionFocus->SetAlignmentInViewport(FVector2D(0.5f, 1.f));
	}

	FVector2D ScreenPosition;
	if (!ProjectWorldLocationToScreen(FocusedBlock->GetInteractionFocusLocation(), ScreenPosition, true))
	{
		ActiveWorldInteractionFocus->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	ActiveWorldInteractionFocus->ShowFocus(
		FocusedBlock->GetBlockDisplayName(),
		bHasPreparedAction ? FormatPreparedWorldInteraction(PreparedOption) : FText::FromString(TEXT("NO USABLE ACTION")));
	ActiveWorldInteractionFocus->SetPositionInViewport(ScreenPosition, false);
	ActiveWorldInteractionFocus->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void AEidosPlayerController::ClearWorldInteractionFocus()
{
	if (AWorldBlockActor* PreviousBlock = Cast<AWorldBlockActor>(FocusedWorldInteractionActor.Get()))
	{
		PreviousBlock->SetInteractionFocused(false);
	}
	FocusedWorldInteractionActor.Reset();
	if (ActiveWorldInteractionFocus)
	{
		ActiveWorldInteractionFocus->SetVisibility(ESlateVisibility::Collapsed);
	}
}

bool AEidosPlayerController::OpenWorldInteractionRadial(AActor* TargetActor)
{
	APageCharacter* SelectedPage = GetSelectedPage();
	if (!SelectedPage || !IsValid(TargetActor)
		|| !TargetActor->GetClass()->ImplementsInterface(UWorldInteractionInterface::StaticClass()))
	{
		return false;
	}

	TArray<FWorldInteractionOption> Options;
	IWorldInteractionInterface::Execute_GetAvailableWorldInteractions(TargetActor, SelectedPage, Options);
	if (Options.IsEmpty())
	{
		return false;
	}

	ContextInteractionTarget = TargetActor;
	ContextInteractionOptions = MoveTemp(Options);
	ShowWorldInteractionRadialWidget(TargetActor, ContextInteractionOptions);
	OnWorldInteractionRadialRequested(TargetActor, ContextInteractionOptions);
	return true;
}

bool AEidosPlayerController::ShowWorldInteractionRadialWidget(AActor* TargetActor, const TArray<FWorldInteractionOption>& Options)
{
	if (!WorldInteractionRadialClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WorldInteraction] WorldInteractionRadialClass is not assigned in %s"), *GetName());
		return false;
	}

	if (ActiveWorldInteractionRadial)
	{
		ActiveWorldInteractionRadial->RemoveFromParent();
		ActiveWorldInteractionRadial = nullptr;
	}

	ActiveWorldInteractionRadial = CreateWidget<UWorldInteractionRadialWidget>(this, WorldInteractionRadialClass);
	if (!ActiveWorldInteractionRadial) return false;

	float CursorX = 0.f;
	float CursorY = 0.f;
	if (!GetMousePosition(CursorX, CursorY))
	{
		CursorX = 640.f;
		CursorY = 360.f;
	}

	ActiveWorldInteractionRadial->AddToViewport(200);
	ActiveWorldInteractionRadial->ShowRadial(this, TargetActor, Options, FVector2D(CursorX, CursorY));

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(ActiveWorldInteractionRadial->TakeWidget());
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	return true;
}

bool AEidosPlayerController::ExecuteContextWorldInteraction(FName InteractionId)
{
	APageCharacter* SelectedPage = GetSelectedPage();
	AActor* TargetActor = ContextInteractionTarget.Get();
	const bool bIsKnownOption = ContextInteractionOptions.ContainsByPredicate(
		[InteractionId](const FWorldInteractionOption& Option) { return Option.InteractionId == InteractionId; });
	const bool bCanExecute = SelectedPage && (!SelectedPage->GetStats() || (!SelectedPage->GetStats()->IsDead() && !SelectedPage->GetStats()->IsDowned() && !SelectedPage->GetStats()->IsRecovering())) && TargetActor && bIsKnownOption
		&& TargetActor->GetClass()->ImplementsInterface(UWorldInteractionInterface::StaticClass());
	if (bCanExecute)
	{
		SelectedPage->BeginManualWorkOverride();
	}
	const bool bSuccess = bCanExecute
		&& IWorldInteractionInterface::Execute_ExecuteWorldInteraction(TargetActor, SelectedPage, InteractionId);
	CloseWorldInteractionRadial();
	return bSuccess;
}

bool AEidosPlayerController::SelectContextWorldInteraction(FName InteractionId)
{
	APageCharacter* SelectedPage = GetSelectedPage();
	AActor* TargetActor = ContextInteractionTarget.Get();
	const bool bIsKnownOption = ContextInteractionOptions.ContainsByPredicate(
		[InteractionId](const FWorldInteractionOption& Option) { return Option.InteractionId == InteractionId; });
	if (!SelectedPage || (SelectedPage->GetStats() && (SelectedPage->GetStats()->IsDead() || SelectedPage->GetStats()->IsDowned() || SelectedPage->GetStats()->IsRecovering())) || !TargetActor || !bIsKnownOption)
	{
		return false;
	}

	SelectedWorldInteractionPage = SelectedPage;
	SelectedWorldInteractionId = InteractionId;
	CloseWorldInteractionRadial();
	UE_LOG(LogTemp, Log, TEXT("[WorldInteraction] Armed '%s' for %s"),
		*InteractionId.ToString(), *GetNameSafe(SelectedPage));
	return true;
}

bool AEidosPlayerController::ExecuteSelectedWorldInteraction()
{
	APageCharacter* SelectedPage = GetSelectedPage();
	if (!SelectedPage || SelectedWorldInteractionPage.Get() != SelectedPage)
	{
		ClearSelectedWorldInteraction();
		return ExecuteDefaultWorldInteraction(FindFocusedWorldInteractionActor());
	}

	// ResolvePreparedWorldInteraction keeps the armed action when this block supports it,
	// and otherwise resolves the focused block's own default action.
	return ExecuteDefaultWorldInteraction(FindFocusedWorldInteractionActor());
}

void AEidosPlayerController::ClearSelectedWorldInteraction()
{
	SelectedWorldInteractionPage.Reset();
	SelectedWorldInteractionId = NAME_None;
}

void AEidosPlayerController::CloseWorldInteractionRadial()
{
	if (ActiveWorldInteractionRadial)
	{
		ActiveWorldInteractionRadial->RemoveFromParent();
		ActiveWorldInteractionRadial = nullptr;
	}
	ContextInteractionTarget.Reset();
	ContextInteractionOptions.Reset();
	OnWorldInteractionRadialClosed();
	RefreshInputModeForCurrentContext();
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

	if (ADungeonCoreActor* DungeonCore = Cast<ADungeonCoreActor>(TargetActor))
	{
		DungeonCore->Interact(this);
		return true;
	}

	if (APageCharacter* DownedPage = Cast<APageCharacter>(TargetActor))
	{
		FString Reason;
		if (UWS_Population* Population = GetWorld() ? GetWorld()->GetSubsystem<UWS_Population>() : nullptr;
			Population && Population->RescueDownedPage(GetSelectedPage(), DownedPage, Reason))
		{
			return true;
		}
		UE_LOG(LogTemp, Verbose, TEXT("[PC] Rescue failed: %s"), *Reason);
		return false;
	}

	if (ADungeonReturnPortalActor* ReturnPortal = Cast<ADungeonReturnPortalActor>(TargetActor))
	{
		ReturnPortal->Interact(this);
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












