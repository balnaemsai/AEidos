#pragma once

#include "CoreMinimal.h"
#include "Save/SaveGameParticipant.h"
#include "Subsystems/WorldSubsystem.h"
#include "Core/Types/WorkTypes.h"
#include "Data/Definitions/ResearchDefinitionRow.h"
#include "WS_Research.generated.h"

class UWS_Work;

USTRUCT(BlueprintType)
struct FResearchView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FName ResearchId;
	UPROPERTY(BlueprintReadOnly) FText DisplayName;
	UPROPERTY(BlueprintReadOnly) FText Description;
	UPROPERTY(BlueprintReadOnly) TArray<FName> PrerequisiteResearchIds;
	UPROPERTY(BlueprintReadOnly) int32 ActiveRequestId = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) int32 ActiveWorkers = 0;
	UPROPERTY(BlueprintReadOnly) int32 MaxWorkers = 0;
	UPROPERTY(BlueprintReadOnly) float Progress = 0.f;
	UPROPERTY(BlueprintReadOnly) float TotalWork = 0.f;
	UPROPERTY(BlueprintReadOnly) bool bCompleted = false;
	UPROPERTY(BlueprintReadOnly) bool bPrerequisitesMet = false;
	UPROPERTY(BlueprintReadOnly) bool bCanStart = false;
	UPROPERTY(BlueprintReadOnly) bool bCanCancel = false;
};

DECLARE_MULTICAST_DELEGATE(FOnResearchChanged);

/** Owns durable one-time research unlocks while using WS_Work for costs, workers and progress. */
UCLASS()
class AEIDOS_API UWS_Research : public UWorldSubsystem, public ISaveGameParticipant
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void WriteToSnapshot_Implementation(FEidosWorldSnapshot& InOutSnapshot) const override;
	virtual void ApplySnapshot_Implementation(const FEidosWorldSnapshot& Snapshot) override;

	void LoadResearchDefs();

	UFUNCTION(BlueprintPure, Category="Research")
	bool HasCompletedResearch(FName ResearchId) const;
	bool HasAllCompletedResearch(const TArray<FName>& ResearchIds) const;

	UFUNCTION(BlueprintPure, Category="Research")
	TArray<FResearchView> GetResearchViews() const;

	UFUNCTION(BlueprintCallable, Category="Research")
	int32 StartResearch(FName ResearchId);

	UFUNCTION(BlueprintCallable, Category="Research")
	bool CancelResearch(FName ResearchId);

	FOnResearchChanged OnResearchChanged;

private:
	const FResearchDefinitionRow* FindDefinition(FName ResearchId) const;
	const FResearchDefinitionRow* FindDefinitionByWorkId(FName WorkId) const;
	bool ArePrerequisitesMet(const FResearchDefinitionRow& Definition) const;
	int32 FindOutstandingRequestId(const FResearchDefinitionRow& Definition) const;
	void HandleWorkRequestStateChanged(int32 RequestId, EWorkRequestLifecycleState NewState);
	void HandleWorkCompleted(int32 RequestId, FName WorkId);

	UPROPERTY() TMap<FName, FResearchDefinitionRow> ResearchDefs;
	UPROPERTY() TSet<FName> CompletedResearchIds;
	TWeakObjectPtr<UWS_Work> WorkSubsystem;
	FDelegateHandle WorkStateChangedHandle;
	FDelegateHandle WorkCompletedHandle;
};
