// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/Camera/CameraModeComponent.h"

#include "Framework/EidosPlayerController.h"
#include "Entities/Page/PageCharacter.h"
#include "FreeCamPawn.h"

#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

UCameraModeComponent::UCameraModeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCameraModeComponent::InitializeForController(AEidosPlayerController* InPC)
{
	PC = InPC;

	// 기본: FollowPage
	ControlMode = ECameraControlMode::FollowPage;
}

void UCameraModeComponent::SetSelectedPage(APageCharacter* NewPage)
{
	SelectedPage = NewPage;

	if (ControlMode == ECameraControlMode::FollowPage)
	{
		EnterFollowPage();
	}
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
	}
}

void UCameraModeComponent::ToggleControlMode()
{
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

		const FVector BaseLoc = (SelectedPage.IsValid())
			                        ? (SelectedPage->GetActorLocation() + FVector(0, 0, 250))
			                        : FVector(0, 0, 400);

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

		// ✅ FreeCam 전용 IMC 추가
		AddIMC(CamPawn->GetFreeCamIMC(), 1);
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
