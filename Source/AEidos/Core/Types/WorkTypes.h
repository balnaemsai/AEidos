#pragma once
#include "CoreMinimal.h"
#include "WorkTypes.generated.h"

UENUM(BlueprintType)
enum class EWorkRequestMode : uint8
{
	Count,
	Until,
	Repeat
};

UENUM(BlueprintType)
enum class EWorkRequestLifecycleState : uint8
{
	Queued,
	Active,
	Completed,
	Cancelled,
	Failed
};

USTRUCT(BlueprintType)
struct FWorkCost
{
	GENERATED_BODY()

	/** Set either ResourceId or ItemId. A row with both fields set is invalid. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName ResourceId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName ItemId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Amount = 0;
};

USTRUCT(BlueprintType)
struct FWorkReward
{
	GENERATED_BODY()

	/** Set either ResourceId or ItemId. Item rewards are deposited into settlement storage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName ResourceId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName ItemId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Amount = 0;
};

USTRUCT(BlueprintType)
struct FWorkRequest
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite) int32 RequestId = 0;
	UPROPERTY(BlueprintReadWrite) FName WorkId;

	UPROPERTY(BlueprintReadWrite) EWorkRequestMode Mode = EWorkRequestMode::Count;

	// Count 모드면 RemainingCount 사용
	UPROPERTY(BlueprintReadWrite) int32 RemainingCount = 1;

	// Until 모드면 목표 리소스/목표량
	UPROPERTY(BlueprintReadWrite) FName UntilResourceId;
	UPROPERTY(BlueprintReadWrite) int32 UntilTargetAmount = 0;

	// 우선순위(큐 내부 정렬에 사용)
	UPROPERTY(BlueprintReadWrite) int32 Priority = 0;
};

USTRUCT(BlueprintType)
struct FWorkInstance
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite) int32 InstanceId = 0;
	UPROPERTY(BlueprintReadWrite) int32 RequestId = 0;
	UPROPERTY(BlueprintReadWrite) FName WorkId;

	UPROPERTY(BlueprintReadWrite) float TotalWork = 0.f;
	UPROPERTY(BlueprintReadWrite) float Progress = 0.f;

	UPROPERTY(BlueprintReadWrite) int32 MaxWorkers = 1;

	// 작업장(간단 MVP: 위치만)
	UPROPERTY(BlueprintReadWrite) FVector SiteLocation = FVector::ZeroVector;

	// 참여자(페이지 ID)
	UPROPERTY(BlueprintReadWrite) TArray<int32> Workers;
};

USTRUCT(BlueprintType)
struct FWorkOrderView
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FName WorkId;
	UPROPERTY(BlueprintReadOnly) FText DisplayName;
	UPROPERTY(BlueprintReadOnly) TArray<FWorkCost> Costs;
	UPROPERTY(BlueprintReadOnly) TArray<FWorkReward> Rewards;
	UPROPERTY(BlueprintReadOnly) int32 QueuedCount = 0;
	UPROPERTY(BlueprintReadOnly) int32 ActiveCount = 0;
	/** Number of Pages currently assigned across active instances of this work type. */
	UPROPERTY(BlueprintReadOnly) int32 ActiveWorkerCount = 0;
	/** Total worker capacity across active instances of this work type. */
	UPROPERTY(BlueprintReadOnly) int32 ActiveMaxWorkers = 0;
	UPROPERTY(BlueprintReadOnly) float ActiveProgress = 0.f;
	UPROPERTY(BlueprintReadOnly) float ActiveTotalWork = 0.f;
	UPROPERTY(BlueprintReadOnly) bool bCanQueue = false;
	/** The most recent outstanding player request for this recipe, whether queued or active. */
	UPROPERTY(BlueprintReadOnly) int32 CancelRequestId = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) bool bCanCancel = false;
};

USTRUCT(BlueprintType)
struct FJob
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite) int32 PageId = 0;
	UPROPERTY(BlueprintReadWrite) int32 InstanceId = 0;

	// 페이지 내부 우선순위(페이지가 여러 job을 가질 때)
	UPROPERTY(BlueprintReadWrite) int32 Priority = 0;

	// 상태
	UPROPERTY(BlueprintReadWrite) bool bIsActive = false;
};



USTRUCT()
struct FWorkProducer
{
	GENERATED_BODY()

	UPROPERTY()
	FName OutputResourceId = "Food";

	/** 게임 시간 1초당 생산량 */
	UPROPERTY()
	float OutputPerGameSecond = 1.0f;
};

USTRUCT(BlueprintType)
struct FJobArray
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	TArray<FJob> Jobs;
};
