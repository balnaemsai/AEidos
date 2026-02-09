// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/GIS_UIRouter.h"

void UGIS_UIRouter::RequestUIState(EUIState NewState)
{
	UE_LOG(LogTemp, Display, TEXT("RequestUIState"));
	if (CurrentState == NewState)
		return;

	const EUIState Prev = CurrentState;
	CurrentState = NewState;

	// 1) 범용 이벤트
	OnUIStateChanged.Broadcast(NewState);

	// 2) 편의 이벤트(선택)
	if (Prev == EUIState::InGame && NewState != EUIState::InGame)
	{
		OnExitInGame.Broadcast();
	}
	if (NewState == EUIState::InGame && Prev != EUIState::InGame)
	{
		OnEnterInGame.Broadcast();
	}
}