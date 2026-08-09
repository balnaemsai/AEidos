#include "UI/HUD/Panels/PageEquipmentEditorWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Data/Definitions/ItemDefinitionRow.h"
#include "Data/GIS_DataRegistry.h"
#include "Engine/GameInstance.h"
#include "Entities/Items/EquipmentComponent.h"
#include "Entities/Items/InventoryComponent.h"
#include "Entities/Page/PageCharacter.h"
#include "UI/HUD/Panels/PageEquipmentItemEntry.h"

namespace
{
	const TCHAR* GetSlotName(EPageEquipmentSlot Slot)
	{
		switch (Slot)
		{
		case EPageEquipmentSlot::LeftHand: return TEXT("왼손");
		case EPageEquipmentSlot::RightHand: return TEXT("오른손");
		case EPageEquipmentSlot::Head: return TEXT("머리");
		case EPageEquipmentSlot::UpperBody: return TEXT("상의");
		case EPageEquipmentSlot::LowerBody: return TEXT("하의");
		case EPageEquipmentSlot::Feet: return TEXT("신발");
		default: return TEXT("장비");
		}
	}
}

void UPageEquipmentEditorWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!EquipmentItemEntryClass)
	{
		EquipmentItemEntryClass = LoadClass<UPageEquipmentItemEntry>(nullptr,
			TEXT("/Game/Blueprints/WBP/WBP_PageEquipmentItemEntry.WBP_PageEquipmentItemEntry_C"));
	}
	if (Button_LeftHand) Button_LeftHand->OnClicked.AddDynamic(this, &UPageEquipmentEditorWidget::HandleLeftHandClicked);
	if (Button_RightHand) Button_RightHand->OnClicked.AddDynamic(this, &UPageEquipmentEditorWidget::HandleRightHandClicked);
	if (Button_Head) Button_Head->OnClicked.AddDynamic(this, &UPageEquipmentEditorWidget::HandleHeadClicked);
	if (Button_UpperBody) Button_UpperBody->OnClicked.AddDynamic(this, &UPageEquipmentEditorWidget::HandleUpperBodyClicked);
	if (Button_LowerBody) Button_LowerBody->OnClicked.AddDynamic(this, &UPageEquipmentEditorWidget::HandleLowerBodyClicked);
	if (Button_Feet) Button_Feet->OnClicked.AddDynamic(this, &UPageEquipmentEditorWidget::HandleFeetClicked);
	if (Button_Close) Button_Close->OnClicked.AddDynamic(this, &UPageEquipmentEditorWidget::HandleCloseClicked);
}

void UPageEquipmentEditorWidget::NativeDestruct()
{
	UnbindObservedComponents();
	Super::NativeDestruct();
}

void UPageEquipmentEditorWidget::OpenForPage(APageCharacter* InPage)
{
	EditedPage = InPage;
	SelectedInventoryItem = NAME_None;
	BindObservedComponents(InPage);
	RefreshEditor();
}

void UPageEquipmentEditorWidget::CloseEditor()
{
	UnbindObservedComponents();
	RemoveFromParent();
}

void UPageEquipmentEditorWidget::RefreshEditor()
{
	APageCharacter* Page = EditedPage.Get();
	if (!Page)
	{
		CloseEditor();
		return;
	}
	if (Text_PageName) Text_PageName->SetText(FText::FromString(GetNameSafe(Page)));
	RefreshEquipmentSlots();
	RebuildInventoryEntries();
	if (Text_SelectedItem)
	{
		Text_SelectedItem->SetText(SelectedInventoryItem.IsNone()
			? FText::FromString(TEXT("장착할 아이템을 선택하세요"))
			: FText::Format(FText::FromString(TEXT("선택: {0}")), GetItemDisplayName(SelectedInventoryItem)));
	}
	if (Text_Instructions)
	{
		Text_Instructions->SetText(FText::FromString(TEXT("아이템 선택 후 호환 슬롯을 클릭해 장착합니다. 장착된 슬롯을 클릭하면 해제합니다.")));
	}
}

void UPageEquipmentEditorWidget::RebuildInventoryEntries()
{
	if (!VerticalBox_InventoryEquipment) return;
	VerticalBox_InventoryEquipment->ClearChildren();
	APageCharacter* Page = EditedPage.Get();
	UInventoryComponent* Inventory = Page ? Page->GetInventory() : nullptr;
	UGIS_DataRegistry* Registry = GetWorld() && GetWorld()->GetGameInstance()
		? GetWorld()->GetGameInstance()->GetSubsystem<UGIS_DataRegistry>() : nullptr;
	if (!Inventory || !Registry || !EquipmentItemEntryClass) return;

	for (const FItemStack& Stack : Inventory->GetStacks())
	{
		const FItemDefinitionRow* Def = Registry->GetItemDef(Stack.ItemId);
		if (!Def || Def->ItemType != EItemType::Equipment) continue;
		UPageEquipmentItemEntry* Entry = CreateWidget<UPageEquipmentItemEntry>(this, EquipmentItemEntryClass);
		if (!Entry) continue;
		Entry->Setup(Stack.ItemId, Def->DisplayName, Stack.Quantity, GetCompatibleSlotsText(Stack.ItemId), Stack.ItemId == SelectedInventoryItem);
		Entry->OnItemClicked.AddDynamic(this, &UPageEquipmentEditorWidget::HandleItemClicked);
		if (UVerticalBoxSlot* EntrySlot = VerticalBox_InventoryEquipment->AddChildToVerticalBox(Entry))
		{
			EntrySlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
		}
	}
}

void UPageEquipmentEditorWidget::RefreshEquipmentSlots()
{
	UEquipmentComponent* Equipment = ObservedEquipment.Get();
	for (int32 Index = 0; Index <= static_cast<int32>(EPageEquipmentSlot::Feet); ++Index)
	{
		const EPageEquipmentSlot EquipmentSlot = static_cast<EPageEquipmentSlot>(Index);
		const FName ItemId = Equipment ? Equipment->GetEquippedItem(EquipmentSlot) : NAME_None;
		if (UTextBlock* Text = GetSlotText(EquipmentSlot))
		{
			Text->SetText(FText::Format(FText::FromString(TEXT("{0}\n{1}")), GetSlotLabel(EquipmentSlot),
				ItemId.IsNone() ? FText::FromString(TEXT("비어 있음")) : GetItemDisplayName(ItemId)));
		}
		if (UButton* Button = GetSlotButton(EquipmentSlot)) Button->SetIsEnabled(true);
	}
}

void UPageEquipmentEditorWidget::HandleSlotClicked(EPageEquipmentSlot EquipmentSlot)
{
	UEquipmentComponent* Equipment = ObservedEquipment.Get();
	if (!Equipment) return;
	if (SelectedInventoryItem.IsNone())
	{
		Equipment->UnequipToInventory(EquipmentSlot);
	}
	else if (Equipment->EquipFromInventory(SelectedInventoryItem, EquipmentSlot))
	{
		SelectedInventoryItem = NAME_None;
	}
	RefreshEditor();
}

void UPageEquipmentEditorWidget::BindObservedComponents(APageCharacter* Page)
{
	UnbindObservedComponents();
	if (!Page) return;
	ObservedEquipment = Page->GetEquipment();
	ObservedInventory = Page->GetInventory();
	if (UEquipmentComponent* Equipment = ObservedEquipment.Get()) Equipment->OnEquipmentChanged.AddDynamic(this, &UPageEquipmentEditorWidget::HandleEquipmentChanged);
	if (UInventoryComponent* Inventory = ObservedInventory.Get()) Inventory->OnInventoryChanged.AddDynamic(this, &UPageEquipmentEditorWidget::HandleInventoryChanged);
}

void UPageEquipmentEditorWidget::UnbindObservedComponents()
{
	if (UEquipmentComponent* Equipment = ObservedEquipment.Get()) Equipment->OnEquipmentChanged.RemoveDynamic(this, &UPageEquipmentEditorWidget::HandleEquipmentChanged);
	if (UInventoryComponent* Inventory = ObservedInventory.Get()) Inventory->OnInventoryChanged.RemoveDynamic(this, &UPageEquipmentEditorWidget::HandleInventoryChanged);
	ObservedEquipment.Reset();
	ObservedInventory.Reset();
}

FText UPageEquipmentEditorWidget::GetItemDisplayName(FName ItemId) const
{
	UGIS_DataRegistry* Registry = GetWorld() && GetWorld()->GetGameInstance()
		? GetWorld()->GetGameInstance()->GetSubsystem<UGIS_DataRegistry>() : nullptr;
	if (const FItemDefinitionRow* Def = Registry ? Registry->GetItemDef(ItemId) : nullptr) return Def->DisplayName;
	return FText::FromName(ItemId);
}

FText UPageEquipmentEditorWidget::GetSlotLabel(EPageEquipmentSlot EquipmentSlot) const
{
	return FText::FromString(GetSlotName(EquipmentSlot));
}

FText UPageEquipmentEditorWidget::GetCompatibleSlotsText(FName ItemId) const
{
	UGIS_DataRegistry* Registry = GetWorld() && GetWorld()->GetGameInstance()
		? GetWorld()->GetGameInstance()->GetSubsystem<UGIS_DataRegistry>() : nullptr;
	const FItemDefinitionRow* Def = Registry ? Registry->GetItemDef(ItemId) : nullptr;
	if (!Def) return FText::GetEmpty();
	TArray<FString> SlotNames;
	for (EPageEquipmentSlot CompatibleSlot : Def->CompatibleEquipmentSlots) SlotNames.Add(GetSlotName(CompatibleSlot));
	return FText::FromString(FString::Join(SlotNames, TEXT(" / ")));
}

UTextBlock* UPageEquipmentEditorWidget::GetSlotText(EPageEquipmentSlot EquipmentSlot) const
{
	switch (EquipmentSlot)
	{
	case EPageEquipmentSlot::LeftHand: return Text_LeftHand;
	case EPageEquipmentSlot::RightHand: return Text_RightHand;
	case EPageEquipmentSlot::Head: return Text_Head;
	case EPageEquipmentSlot::UpperBody: return Text_UpperBody;
	case EPageEquipmentSlot::LowerBody: return Text_LowerBody;
	case EPageEquipmentSlot::Feet: return Text_Feet;
	default: return nullptr;
	}
}

UButton* UPageEquipmentEditorWidget::GetSlotButton(EPageEquipmentSlot EquipmentSlot) const
{
	switch (EquipmentSlot)
	{
	case EPageEquipmentSlot::LeftHand: return Button_LeftHand;
	case EPageEquipmentSlot::RightHand: return Button_RightHand;
	case EPageEquipmentSlot::Head: return Button_Head;
	case EPageEquipmentSlot::UpperBody: return Button_UpperBody;
	case EPageEquipmentSlot::LowerBody: return Button_LowerBody;
	case EPageEquipmentSlot::Feet: return Button_Feet;
	default: return nullptr;
	}
}

void UPageEquipmentEditorWidget::HandleLeftHandClicked() { HandleSlotClicked(EPageEquipmentSlot::LeftHand); }
void UPageEquipmentEditorWidget::HandleRightHandClicked() { HandleSlotClicked(EPageEquipmentSlot::RightHand); }
void UPageEquipmentEditorWidget::HandleHeadClicked() { HandleSlotClicked(EPageEquipmentSlot::Head); }
void UPageEquipmentEditorWidget::HandleUpperBodyClicked() { HandleSlotClicked(EPageEquipmentSlot::UpperBody); }
void UPageEquipmentEditorWidget::HandleLowerBodyClicked() { HandleSlotClicked(EPageEquipmentSlot::LowerBody); }
void UPageEquipmentEditorWidget::HandleFeetClicked() { HandleSlotClicked(EPageEquipmentSlot::Feet); }
void UPageEquipmentEditorWidget::HandleItemClicked(FName ItemId) { SelectedInventoryItem = ItemId; RefreshEditor(); }
void UPageEquipmentEditorWidget::HandleEquipmentChanged() { RefreshEditor(); }
void UPageEquipmentEditorWidget::HandleInventoryChanged() { RefreshEditor(); }
void UPageEquipmentEditorWidget::HandleCloseClicked() { CloseEditor(); }
