// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Entities/Page/PageCharacter.h"
#include "CameraModeComponent.generated.h"

class AEidosPlayerController;
class APageCharacter;
class AFreeCamPawn;
class UInputMappingContext;
class USpringArmComponent;

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

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void OnRegister() override;
	
	void AddOrbitYawInput(float Axis);
	void AddZoomInput(float Axis);
	
	void InitializeForController(AEidosPlayerController* InPC);

	// 외부(컨트롤러/UI)에서 선택 페이지 바꾸기
	void SetSelectedPage(APageCharacter* NewPage);

	// 단축키 핸들러
	void ToggleViewMode();      // V
	void ToggleControlMode();   // Y

	ECameraControlMode GetControlMode() const { return ControlMode; }
	EPageViewMode GetViewMode() const {return SelectedPage->GetViewMode();}
	TObjectPtr<APageCharacter> GetSelectedPage() const { return SelectedPage; }
	float GetOrbitYawWorldDeg() {return OrbitYawWorldDeg;}

	UFUNCTION(BlueprintCallable)
	void EnterThirdPerson(bool bForceApply);

	UFUNCTION()
	USceneComponent* ResolveActiveThirdPersonPivot() const;

protected:
	UPROPERTY()
	TWeakObjectPtr<AEidosPlayerController> PC;

	UPROPERTY()
	TObjectPtr<APageCharacter> SelectedPage;

	UPROPERTY()
	TWeakObjectPtr<AFreeCamPawn> FreeCamPawn;

	UPROPERTY(EditDefaultsOnly, Category="Camera|FreeCam")
	TSubclassOf<AFreeCamPawn> FreeCamPawnClass;

private:
	void EnterFollowPage();
	void EnterFreeCam();

	void AddIMC(UInputMappingContext* IMC, int32 Priority);
	void RemoveIMC(UInputMappingContext* IMC);

	ECameraControlMode ControlMode = ECameraControlMode::FollowPage;
	
	USpringArmComponent* ResolveActiveThirdPersonArm() const;

	UPROPERTY(EditAnywhere, Category="Camera|ThirdPerson")
	float OrbitYawSpeedDegPerSec = 90.f; // Q/E 누르면 초당 90도

	UPROPERTY(EditAnywhere, Category="Camera|ThirdPerson")
	float ZoomStep = 120.f;              // 휠 1틱당 이동량(원하면 조절)

	UPROPERTY(EditAnywhere, Category="Camera|ThirdPerson")
	float ArmLengthMin = 250.f;

	UPROPERTY(EditAnywhere, Category="Camera|ThirdPerson")
	float ArmLengthMax = 900.f;

	// 줌에 따라 Pitch를 바꾸는 범위 (네 그림 느낌: 멀수록 위에서, 가까울수록 낮은 각)
	UPROPERTY(EditAnywhere, Category="Camera|ThirdPerson")
	float PitchAtZoomOut = -60.f; // ArmLengthMax일 때

	UPROPERTY(EditAnywhere, Category="Camera|ThirdPerson")
	float PitchAtZoomIn  = -20.f; // ArmLengthMin일 때

	UPROPERTY(EditAnywhere, Category="Camera|ThirdPerson")
	float ArmInterpSpeed = 12.f;

	UPROPERTY(EditAnywhere, Category="Camera|ThirdPerson")
	float PitchInterpSpeed = 12.f;

	float DesiredArmLength = 600.f;
	float DesiredPitch = -45.f;
	float PendingYawAxis = 0.f;
	float PendingZoomAxis = 0.f;
	float OrbitYawWorldDeg = 0.f;
};
