// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/Panels/Panel_Dungeons.h"
#include "Framework/EidosPlayerController.h"
#include "Entities/Page/PageCharacter.h"
#include "World/Settlement/WS_PortalDirector.h"

void UPanel_Dungeons::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshFromWorld();
}

void UPanel_Dungeons::RefreshFromWorld()
{
	CachedPortals.Reset();
	bSelectedPageInDungeon = false;

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (UWS_PortalDirector* PortalDirector = World->GetSubsystem<UWS_PortalDirector>())
	{
		CachedPortals = PortalDirector->GetActivePortals();
	}

	if (AEidosPlayerController* EidosPC = Cast<AEidosPlayerController>(GetOwningPlayer()))
	{
		if (APageCharacter* SelectedPage = EidosPC->GetSelectedPage())
		{
			bSelectedPageInDungeon = SelectedPage->IsInDungeon();
		}
	}
}

