#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ResourceDefinitionRow.generated.h"

USTRUCT(BlueprintType)
struct FResourceDefinitionRow : public FTableRowBase
{
	GENERATED_BODY()

	/** UI 표시용 이름 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	/** 새 게임 시작 시 기본 보유량 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 DefaultStartingAmount = 0;

	/** 저장/로드 대상 자원인지 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bSavable = true;

	/** 필요하면 아이콘, 색상, 정렬 우선순위 등도 나중에 추가 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 SortOrder = 0;

	// Every resource row is a valid warehouse resource. New resources only need
	// a DataTable row; no code-side enum or whitelist is required.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.0"))
	float UnitWeight = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.0"))
	float UnitVolume = 1.f;
};
