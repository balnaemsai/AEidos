#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ResearchDefinitionRow.generated.h"

/** A one-time settlement unlock. The actual effort/cost is defined by ResearchWorkId in DT_Work. */
USTRUCT(BlueprintType)
struct FResearchDefinitionRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName ResearchId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(MultiLine=true))
	FText Description;

	/** A DT_Work row with WorkCategory set to Research. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName ResearchWorkId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FName> PrerequisiteResearchIds;
};
