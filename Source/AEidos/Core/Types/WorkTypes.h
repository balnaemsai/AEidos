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

/** Broad work groups that Pages can prioritize independently. */
UENUM(BlueprintType)
enum class EWorkCategory : uint8
{
	Craft,
	Gather,
	Construction,
	Research
};

/** A Page's preference for one work group. Zero disables automatic assignment. */
USTRUCT(BlueprintType)
struct FPageWorkPriority
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EWorkCategory WorkCategory = EWorkCategory::Craft;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="5"))
	int32 Priority = 3;
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

	/** Page 고유 Job일 때 이 요청을 수행해야 하는 Page. 공유 Job에서는 INDEX_NONE이다. */
	UPROPERTY(BlueprintReadWrite) int32 TargetPageId = INDEX_NONE;
};

USTRUCT(BlueprintType)
struct FWorkInstance
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite) int32 InstanceId = 0;
	UPROPERTY(BlueprintReadWrite) int32 RequestId = 0;
	UPROPERTY(BlueprintReadWrite) FName WorkId;

	/** Request priority used as a tie breaker after the Page's category preference. */
	UPROPERTY(BlueprintReadWrite) int32 Priority = 0;

	/** Page 고유 Job의 소유 Page. 공유 Job에서는 INDEX_NONE이다. */
	UPROPERTY(BlueprintReadWrite) int32 TargetPageId = INDEX_NONE;

	UPROPERTY(BlueprintReadWrite) float TotalWork = 0.f;
	UPROPERTY(BlueprintReadWrite) float Progress = 0.f;

	UPROPERTY(BlueprintReadWrite) int32 MaxWorkers = 1;

	// 작업장(간단 MVP: 위치만)
	UPROPERTY(BlueprintReadWrite) FVector SiteLocation = FVector::ZeroVector;

	/** Only completed-building work sites relocate assigned Pages. World work stays at its existing location. */
	UPROPERTY(BlueprintReadWrite) bool bTeleportWorkersToSite = false;

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
	/** A named completed building facility required before this order can begin. */
	UPROPERTY(BlueprintReadOnly) FName RequiredSiteTag;
	UPROPERTY(BlueprintReadOnly) bool bHasRequiredSite = true;
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
