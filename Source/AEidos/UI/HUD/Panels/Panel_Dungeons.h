// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/Types/PortalTypes.h"
#include "Panel_Dungeons.generated.h"

/**
 * 
 */
UCLASS()
class AEIDOS_API UPanel_Dungeons : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category="Dungeons")
	void RefreshFromWorld();

	UFUNCTION(BlueprintPure, Category="Dungeons")
	const TArray<FPortalState>& GetCachedPortals() const { return CachedPortals; }

	UFUNCTION(BlueprintPure, Category="Dungeons")
	bool IsSelectedPageInDungeon() const { return bSelectedPageInDungeon; }

protected:
	UPROPERTY(BlueprintReadOnly, Category="Dungeons")
	TArray<FPortalState> CachedPortals;

	UPROPERTY(BlueprintReadOnly, Category="Dungeons")
	bool bSelectedPageInDungeon = false;
};
