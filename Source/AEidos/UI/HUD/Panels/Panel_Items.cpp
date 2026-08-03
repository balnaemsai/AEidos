// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/Panels/Panel_Items.h"

#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Data/Definitions/ItemDefinitionRow.h"
#include "Data/Definitions/ResourceDefinitionRow.h"
#include "Data/GIS_DataRegistry.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Entities/Items/InventoryComponent.h"
#include "Entities/Page/PageCharacter.h"
#include "Framework/EidosPlayerController.h"
#include "World/Settlement/WS_Economy.h"
#include "World/Settlement/WS_ItemStorage.h"

void UPanel_Items::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshFromWorld();
}

void UPanel_Items::NativeDestruct()
{
	StopAutoRefresh();
	Super::NativeDestruct();
}

void UPanel_Items::OnPanelShown_Implementation()
{
	RefreshFromWorld();
	StartAutoRefresh();
}

void UPanel_Items::OnPanelHidden_Implementation()
{
	StopAutoRefresh();
}

void UPanel_Items::RefreshFromWorld()
{
	CachedResourceViews.Reset();
	CachedStoredItems.Reset();
	CachedSelectedPageItems.Reset();

	UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	UGIS_DataRegistry* Registry = GameInstance ? GameInstance->GetSubsystem<UGIS_DataRegistry>() : nullptr;
	UWS_Economy* Economy = World ? World->GetSubsystem<UWS_Economy>() : nullptr;
	UWS_ItemStorage* Storage = World ? World->GetSubsystem<UWS_ItemStorage>() : nullptr;
	if (!Registry || !Registry->EnsureReadySync() || !Economy || !Storage)
	{
		RebuildListWidgets();
		return;
	}

	for (const FName ResourceId : Registry->GetAllResourceIds())
	{
		const FResourceDefinitionRow* Def = Registry->GetResourceDef(ResourceId);
		if (!Def)
		{
			continue;
		}
		FStoredResourceView& View = CachedResourceViews.AddDefaulted_GetRef();
		View.ResourceId = ResourceId;
		View.DisplayName = Def->DisplayName;
		View.Amount = Economy->GetAmount(ResourceId);
	}
	CachedResourceViews.Sort([](const FStoredResourceView& A, const FStoredResourceView& B)
	{
		return A.ResourceId.LexicalLess(B.ResourceId);
	});

	CachedStoredItems = Storage->GetStoredItems();
	if (const AEidosPlayerController* PC = Cast<AEidosPlayerController>(GetOwningPlayer()))
	{
		if (const APageCharacter* SelectedPage = PC->GetSelectedPage())
		{
			if (const UInventoryComponent* Inventory = SelectedPage->GetInventory())
			{
				CachedSelectedPageItems = Inventory->GetStacks();
			}
		}
	}

	if (Text_StorageWeight)
	{
		Text_StorageWeight->SetText(FText::Format(FText::FromString(TEXT("창고 무게 {0} / {1}")),
			FMath::RoundToInt(Storage->GetCurrentWeight()), FMath::RoundToInt(Storage->GetTotalWeightCapacity())));
	}
	if (Text_StorageVolume)
	{
		Text_StorageVolume->SetText(FText::Format(FText::FromString(TEXT("창고 부피 {0} / {1}")),
			FMath::RoundToInt(Storage->GetCurrentVolume()), FMath::RoundToInt(Storage->GetTotalVolumeCapacity())));
	}
	if (Text_SelectedPageInventory)
	{
		const APageCharacter* SelectedPage = Cast<AEidosPlayerController>(GetOwningPlayer()) ? Cast<AEidosPlayerController>(GetOwningPlayer())->GetSelectedPage() : nullptr;
		Text_SelectedPageInventory->SetText(SelectedPage
			? FText::Format(FText::FromString(TEXT("{0} 휴대품  {1}/{2} 부피  {3}/{4} 무게")),
				FText::FromString(SelectedPage->GetName()),
				FMath::RoundToInt(SelectedPage->GetCurrentInventoryVolume()), FMath::RoundToInt(SelectedPage->GetMaxInventoryVolume()),
				FMath::RoundToInt(SelectedPage->GetCurrentInventoryWeight()), FMath::RoundToInt(SelectedPage->GetMaxInventoryWeight()))
			: FText::FromString(TEXT("선택된 Page 없음")));
	}

	RebuildListWidgets();
}

void UPanel_Items::StartAutoRefresh()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(RefreshTimerHandle, this, &UPanel_Items::RefreshFromWorld, RefreshIntervalSeconds, true);
	}
}

void UPanel_Items::StopAutoRefresh()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RefreshTimerHandle);
	}
}

void UPanel_Items::RebuildListWidgets()
{
	auto AddLine = [](UVerticalBox* Box, const FText& Text)
	{
		if (!Box)
		{
			return;
		}
		UTextBlock* Line = NewObject<UTextBlock>(Box);
		Line->SetText(Text);
		Line->SetColorAndOpacity(FSlateColor(FLinearColor(0.84f, 0.86f, 0.86f)));
		Line->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 14));
		Box->AddChildToVerticalBox(Line);
	};

	if (VerticalBox_ResourceEntries)
	{
		VerticalBox_ResourceEntries->ClearChildren();
		for (const FStoredResourceView& Resource : CachedResourceViews)
		{
			AddLine(VerticalBox_ResourceEntries, FText::Format(FText::FromString(TEXT("{0}  {1}")), Resource.DisplayName, FText::AsNumber(Resource.Amount)));
		}
	}

	if (VerticalBox_StoredItemEntries)
	{
		VerticalBox_StoredItemEntries->ClearChildren();
		for (const FItemStack& Stack : CachedStoredItems)
		{
			AddLine(VerticalBox_StoredItemEntries, FText::Format(FText::FromString(TEXT("{0} x{1}")), FText::FromName(Stack.ItemId), FText::AsNumber(Stack.Quantity)));
		}
	}

	if (VerticalBox_PageItemEntries)
	{
		VerticalBox_PageItemEntries->ClearChildren();
		for (const FItemStack& Stack : CachedSelectedPageItems)
		{
			AddLine(VerticalBox_PageItemEntries, FText::Format(FText::FromString(TEXT("{0} x{1}")), FText::FromName(Stack.ItemId), FText::AsNumber(Stack.Quantity)));
		}
	}

	if (Text_EmptyStoredItems)
	{
		Text_EmptyStoredItems->SetVisibility(CachedStoredItems.IsEmpty() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (Text_EmptyPageItems)
	{
		Text_EmptyPageItems->SetVisibility(CachedSelectedPageItems.IsEmpty() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

