// Fill out your copyright notice in the Description page of Project Settings.


#include "Save/GIS_SaveLoad.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Save/EidosSaveGame.h"
#include "Save/SaveGameParticipant.h"

#include "World/Settlement/WS_SettlementSpace.h"
#include "Simulation/WS_SimulationOrchestrator.h"

DEFINE_LOG_CATEGORY(LogSaveLoad);

void UGIS_SaveLoad::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	bHasPendingSnapshot = false;
	bHasNewGameSnapshot = false;
	PendingSnapshot = FEidosWorldSnapshot{};
	NewGameSnapshot = FEidosWorldSnapshot{};
}

void UGIS_SaveLoad::Deinitialize()
{
	bHasPendingSnapshot = false;
	bHasNewGameSnapshot = false;
	Super::Deinitialize();
}

void UGIS_SaveLoad::SetPendingSnapshot(const FEidosWorldSnapshot& InSnapshot)
{
	PendingSnapshot = InSnapshot;
	bHasPendingSnapshot = true;
}

void UGIS_SaveLoad::ClearPendingSnapshot()
{
	PendingSnapshot = FEidosWorldSnapshot{};
	bHasPendingSnapshot = false;
}

void UGIS_SaveLoad::BuildNewGameSnapshotIfNeeded(const FString& MapNameHint)
{
	if (bHasNewGameSnapshot)
	{
		return;
	}

	FEidosWorldSnapshot S;
	S.SchemaVersion = 1;
	S.MapName = MapNameHint;
	S.SavedAtUtc = FDateTime::UtcNow();

	// 최소 초기값(원하면 시나리오/난이도/시드 등 저장)
	S.SetKVString(TEXT("Game.Mode"), TEXT("NewGame"));
	S.SetKVString(TEXT("World.Seed"), TEXT("12345"));

	NewGameSnapshot = S;
	bHasNewGameSnapshot = true;
}

void UGIS_SaveLoad::ApplyPendingSnapshotToWorld(UWorld& World)
{
	if (bHasPendingSnapshot)
	{
		UE_LOG(LogSaveLoad, Log, TEXT("[SaveLoad] ApplyPendingSnapshotToWorld START. Map=%s"), *World.GetMapName());
		DispatchApplySnapshot(World, PendingSnapshot);
		ClearPendingSnapshot();
		UE_LOG(LogSaveLoad, Log, TEXT("[SaveLoad] ApplyPendingSnapshotToWorld DONE (Pending)."));
		return;
	}
	
	if (!bHasNewGameSnapshot)
	{
		BuildNewGameSnapshotIfNeeded(World.GetMapName());
	}

	UE_LOG(LogSaveLoad, Log, TEXT("[SaveLoad] No PendingSnapshot. Apply NewGameSnapshot. Map=%s"), *World.GetMapName());
	DispatchApplySnapshot(World, NewGameSnapshot);
}

bool UGIS_SaveLoad::SaveToSlot(UWorld& World, const FString& SlotName, int32 UserIndex)
{
	UEidosSaveGame* SaveObj = Cast<UEidosSaveGame>(UGameplayStatics::CreateSaveGameObject(UEidosSaveGame::StaticClass()));
	if (!SaveObj)
	{
		UE_LOG(LogSaveLoad, Error, TEXT("[SaveLoad] CreateSaveGameObject failed."));
		return false;
	}

	SaveObj->Snapshot = CaptureWorldSnapshot(World);

	const bool bOk = UGameplayStatics::SaveGameToSlot(SaveObj, SlotName, UserIndex);
	UE_LOG(LogSaveLoad, Log, TEXT("[SaveLoad] SaveToSlot '%s' => %s"), *SlotName, bOk ? TEXT("OK") : TEXT("FAIL"));
	return bOk;
}

bool UGIS_SaveLoad::LoadFromSlotToPending(const FString& SlotName, int32 UserIndex)
{
	if (!UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
	{
		UE_LOG(LogSaveLoad, Warning, TEXT("[SaveLoad] Slot not found: %s"), *SlotName);
		return false;
	}

	USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex);
	UEidosSaveGame* LoadedObj = Cast<UEidosSaveGame>(Loaded);
	if (!LoadedObj)
	{
		UE_LOG(LogSaveLoad, Error, TEXT("[SaveLoad] Loaded save is not UEidosSaveGameSlot. Slot=%s"), *SlotName);
		return false;
	}

	SetPendingSnapshot(LoadedObj->Snapshot);
	UE_LOG(LogSaveLoad, Log, TEXT("[SaveLoad] LoadFromSlotToPending '%s' OK. Map=%s"),
		*SlotName, *LoadedObj->Snapshot.MapName);

	return true;
}

FEidosWorldSnapshot UGIS_SaveLoad::CaptureWorldSnapshot(UWorld& World) const
{
	FEidosWorldSnapshot S;
	S.SchemaVersion = 1;
	S.MapName = World.GetMapName();
	S.SavedAtUtc = FDateTime::UtcNow();

	// 1) 월드 서브시스템 스냅샷(명시적)
	{
		// 예시: SettlementSpace
		if (UWS_SettlementSpace* SS = World.GetSubsystem<UWS_SettlementSpace>())
		{
			if (SS->GetClass()->ImplementsInterface(USaveGameParticipant::StaticClass()))
			{
				ISaveGameParticipant::Execute_WriteToSnapshot(SS, S);
			}
		}

		// 예시: SimulationOrchestrator (시간/속도 등 저장하고 싶다면)
		if (UWS_SimulationOrchestrator* Orch = World.GetSubsystem<UWS_SimulationOrchestrator>())
		{
			if (Orch->GetClass()->ImplementsInterface(USaveGameParticipant::StaticClass()))
			{
				ISaveGameParticipant::Execute_WriteToSnapshot(Orch, S);
			}
		}

		// TODO: 저장 참여시키고 싶은 WS를 여기 계속 추가
	}

	for (TActorIterator<AActor> It(&World); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor))
		{
			continue;
		}

		if (Actor->GetClass()->ImplementsInterface(USaveGameParticipant::StaticClass()))
		{
			ISaveGameParticipant::Execute_WriteToSnapshot(Actor, S);
		}
	}

	return S;
}

void UGIS_SaveLoad::DispatchApplySnapshot(UWorld& World, const FEidosWorldSnapshot& Snapshot)
{
	// 0) (선택) 적용 전/후 로그
	UE_LOG(LogSaveLoad, Log, TEXT("[SaveLoad] DispatchApplySnapshot Map=%s KV=%d"),
		*World.GetMapName(), Snapshot.KV.Num());

	// 1) WS에 먼저 Apply (보통 “월드 전역 상태”가 Actor보다 선행되는 게 안정적)
	{
		if (UWS_SettlementSpace* SS = World.GetSubsystem<UWS_SettlementSpace>())
		{
			if (SS->GetClass()->ImplementsInterface(USaveGameParticipant::StaticClass()))
			{
				ISaveGameParticipant::Execute_ApplySnapshot(SS, Snapshot);
			}
		}

		if (UWS_SimulationOrchestrator* Orch = World.GetSubsystem<UWS_SimulationOrchestrator>())
		{
			if (Orch->GetClass()->ImplementsInterface(USaveGameParticipant::StaticClass()))
			{
				ISaveGameParticipant::Execute_ApplySnapshot(Orch, Snapshot);
			}
		}

		// TODO: WS_Building, WS_Population, WS_Work 등도 스냅샷을 반영해야 한다면 여기 추가
	}

	// 2) Actor Apply (인터페이스 구현자만)
	for (TActorIterator<AActor> It(&World); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor))
		{
			continue;
		}

		if (Actor->GetClass()->ImplementsInterface(USaveGameParticipant::StaticClass()))
		{
			ISaveGameParticipant::Execute_ApplySnapshot(Actor, Snapshot);
		}
	}

	// 3) (선택) 후처리: 예를 들어 “스폰/링크/경로 재빌드” 같은 것을
	// WorldBootstrap에서 따로 Orchestrator 단계로 넘겨도 됨.
}










