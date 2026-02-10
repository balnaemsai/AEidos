// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CameraModeComponent.generated.h"

class AEidosPlayerController;
class APageCharacter;
class AFreeCamPawn;
class UInputMappingContext;

UENUM()
enum class ECameraControlMode : uint8
{
	FollowPage,
	FreeCam
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class AEIDOS_API UCameraModeComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCameraModeComponent();

	void InitializeForController(AEidosPlayerController* InPC);

	// 외부(컨트롤러/UI)에서 선택 페이지 바꾸기
	void SetSelectedPage(APageCharacter* NewPage);

	// 단축키 핸들러
	void ToggleViewMode();      // V
	void ToggleControlMode();   // Y

	ECameraControlMode GetControlMode() const { return ControlMode; }

protected:
	UPROPERTY()
	TWeakObjectPtr<AEidosPlayerController> PC;

	UPROPERTY()
	TWeakObjectPtr<APageCharacter> SelectedPage;

	UPROPERTY()
	TWeakObjectPtr<AFreeCamPawn> FreeCamPawn;

	UPROPERTY(EditDefaultsOnly, Category="Camera|FreeCam")
	TSubclassOf<AFreeCamPawn> FreeCamPawnClass;

private:
	void EnterFollowPage();
	void EnterFreeCam();

	void AddIMC(UInputMappingContext* IMC, int32 Priority);
	void RemoveIMC(UInputMappingContext* IMC);

private:
	ECameraControlMode ControlMode = ECameraControlMode::FollowPage;
};
