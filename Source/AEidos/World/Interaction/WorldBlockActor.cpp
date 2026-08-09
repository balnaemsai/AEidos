#include "World/Interaction/WorldBlockActor.h"

#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Data/GIS_DataRegistry.h"
#include "Data/Definitions/BlockDefinitionRow.h"
#include "Data/Definitions/BlockInteractionDefinitionRow.h"
#include "Entities/Items/EquipmentComponent.h"
#include "Entities/Items/InventoryComponent.h"
#include "Entities/Page/PageCharacter.h"

AWorldBlockActor::AWorldBlockActor()
{
	PrimaryActorTick.bCanEverTick = false;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionObjectType(ECC_WorldDynamic);
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PreviewOverlayFinder(
		TEXT("/Engine/EngineDebugMaterials/M_SimpleTranslucent.M_SimpleTranslucent"));
	if (PreviewOverlayFinder.Succeeded())
	{
		PlacementPreviewOverlayMaterial = PreviewOverlayFinder.Object;
	}
}

void AWorldBlockActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyBlockDefinition();
}

void AWorldBlockActor::BeginPlay()
{
	Super::BeginPlay();
	ApplyBlockDefinition();
}

void AWorldBlockActor::ApplyBlockDefinition()
{
	UGameInstance* GameInstance = GetGameInstance();
	UGIS_DataRegistry* DataRegistry = GameInstance
		? GameInstance->GetSubsystem<UGIS_DataRegistry>()
		: nullptr;
	if (!DataRegistry || !DataRegistry->EnsureReadySync())
	{
		// Construction can run in the editor, where the game-instance registry is unavailable.
		return;
	}

	if (const FBlockDefinitionRow* BlockRow = DataRegistry->GetBlockDef(BlockId))
	{
		BlockId = BlockRow->BlockId.IsNone() ? BlockId : BlockRow->BlockId;
		BlockDisplayName = BlockRow->DisplayName;
		RemainingIntegrity = FMath::Max(1, BlockRow->MaxIntegrity);
		bDestroyWhenDepleted = BlockRow->bDestroyWhenDepleted;

		Interactions.Reset();
		TArray<const FBlockInteractionDefinitionRow*> InteractionRows;
		DataRegistry->GetBlockInteractions(BlockId, InteractionRows);
		for (const FBlockInteractionDefinitionRow* InteractionRow : InteractionRows)
		{
			FWorldBlockInteractionDefinition& Interaction = Interactions.AddDefaulted_GetRef();
			Interaction.InteractionId = InteractionRow->InteractionId;
			Interaction.DisplayName = InteractionRow->DisplayName;
			Interaction.Description = InteractionRow->Description;
			Interaction.RequiredToolTag = InteractionRow->RequiredToolTag;
			Interaction.ResultItemId = InteractionRow->ResultItemId;
			Interaction.ResultQuantity = FMath::Max(1, InteractionRow->ResultQuantity);
			Interaction.IntegrityDamage = FMath::Max(1, InteractionRow->IntegrityDamage);
			Interaction.bIsDefault = InteractionRow->bIsDefault;
			Interaction.bConsumesIntegrity = InteractionRow->bConsumesIntegrity;
		}
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[WorldBlock] Missing DT_Block row for BlockId=%s"), *BlockId.ToString());
}

bool AWorldBlockActor::CanInteract(APageCharacter* InteractingPage) const
{
	return InteractingPage && RemainingIntegrity > 0
		&& FVector::DistSquared(InteractingPage->GetActorLocation(), GetActorLocation()) <= FMath::Square(InteractionRangeCm);
}

bool AWorldBlockActor::HasRequiredTool(APageCharacter* InteractingPage, FName RequiredToolTag) const
{
	return RequiredToolTag.IsNone() || (InteractingPage && InteractingPage->GetEquipment()
		&& InteractingPage->GetEquipment()->HasActiveToolTag(RequiredToolTag));
}

void AWorldBlockActor::GetBlockInteractionDefinitions(TArray<FWorldBlockInteractionDefinition>& OutDefinitions) const
{
	OutDefinitions = Interactions;
}

FText AWorldBlockActor::GetBlockDisplayName() const
{
	return BlockDisplayName.IsEmpty() ? FText::FromName(GetBlockId()) : BlockDisplayName;
}

void AWorldBlockActor::SetInteractionFocused(bool bFocused)
{
	if (Mesh)
	{
		// Stencil 1 is reserved for the currently focused world block by the outline post-process.
		Mesh->SetCustomDepthStencilValue(1);
		Mesh->SetRenderCustomDepth(bFocused);
	}
}

FVector AWorldBlockActor::GetInteractionFocusLocation() const
{
	const float MeshTop = Mesh ? Mesh->Bounds.BoxExtent.Z : 0.f;
	return GetActorLocation() + FVector(0.f, 0.f, MeshTop + FocusLabelOffsetCm);
}

void AWorldBlockActor::SetPlacementPreview(bool bInPreviewMode, bool bInPlacementValid)
{
	bPlacementPreviewMode = bInPreviewMode;
	if (!Mesh)
	{
		return;
	}

	Mesh->SetMobility(bInPreviewMode ? EComponentMobility::Movable : EComponentMobility::Static);
	Mesh->SetCollisionEnabled(bInPreviewMode ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryOnly);
	if (!bInPreviewMode)
	{
		Mesh->SetOverlayMaterial(nullptr);
		return;
	}

	if (!PlacementPreviewOverlayMID && PlacementPreviewOverlayMaterial)
	{
		PlacementPreviewOverlayMID = UMaterialInstanceDynamic::Create(PlacementPreviewOverlayMaterial, this);
	}
	if (PlacementPreviewOverlayMID)
	{
		const FLinearColor PreviewColor = bInPlacementValid
			? FLinearColor(0.45f, 0.85f, 1.f, 1.f)
			: FLinearColor(1.f, 0.35f, 0.35f, 1.f);
		PlacementPreviewOverlayMID->SetVectorParameterValue(TEXT("Color"), PreviewColor);
		PlacementPreviewOverlayMID->SetVectorParameterValue(TEXT("BaseColor"), PreviewColor);
		PlacementPreviewOverlayMID->SetVectorParameterValue(TEXT("TintColor"), PreviewColor);
		PlacementPreviewOverlayMID->SetVectorParameterValue(TEXT("EmissiveColor"), PreviewColor);
		PlacementPreviewOverlayMID->SetScalarParameterValue(TEXT("Opacity"), bInPlacementValid ? 0.4f : 0.5f);
		Mesh->SetOverlayMaterial(PlacementPreviewOverlayMID);
	}
}

FVector AWorldBlockActor::GetPlacementBoundsExtent() const
{
	return Mesh && !Mesh->Bounds.BoxExtent.IsNearlyZero()
		? Mesh->Bounds.BoxExtent
		: FVector(50.f);
}

void AWorldBlockActor::GetAvailableWorldInteractions_Implementation(APageCharacter* InteractingPage,
	TArray<FWorldInteractionOption>& OutOptions)
{
	OutOptions.Reset();
	if (!CanInteract(InteractingPage)) return;

	for (const FWorldBlockInteractionDefinition& Definition : Interactions)
	{
		if (Definition.InteractionId.IsNone()) continue;
		FWorldInteractionOption& Option = OutOptions.AddDefaulted_GetRef();
		Option.InteractionId = Definition.InteractionId;
		Option.DisplayName = Definition.DisplayName.IsEmpty() ? FText::FromName(Definition.InteractionId) : Definition.DisplayName;
		Option.Description = Definition.Description;
		Option.RequiredToolTag = Definition.RequiredToolTag;
		Option.bIsDefault = Definition.bIsDefault;
	}
}

bool AWorldBlockActor::GrantInteractionResult(APageCharacter* InteractingPage, const FWorldBlockInteractionDefinition& Interaction)
{
	if (Interaction.ResultItemId.IsNone())
	{
		// State-only interactions such as opening a door do not award an inventory item.
		return true;
	}
	UInventoryComponent* Inventory = InteractingPage ? InteractingPage->GetInventory() : nullptr;
	return Inventory && Inventory->TryAddItem(Interaction.ResultItemId, Interaction.ResultQuantity) == Interaction.ResultQuantity;
}

bool AWorldBlockActor::ExecuteWorldInteraction_Implementation(APageCharacter* InteractingPage, FName InteractionId)
{
	if (!CanInteract(InteractingPage)) return false;
	const FWorldBlockInteractionDefinition* Definition = Interactions.FindByPredicate(
		[InteractionId](const FWorldBlockInteractionDefinition& Entry) { return Entry.InteractionId == InteractionId; });
	if (!Definition || !HasRequiredTool(InteractingPage, Definition->RequiredToolTag)) return false;
	if (!GrantInteractionResult(InteractingPage, *Definition)) return false;

	if (Definition->bConsumesIntegrity)
	{
		RemainingIntegrity = FMath::Max(0, RemainingIntegrity - FMath::Max(1, Definition->IntegrityDamage));
		UE_LOG(LogTemp, Log, TEXT("[WorldBlock] Interaction=%s Block=%s Result=%s x%d Integrity=%d"),
			*Definition->InteractionId.ToString(), *GetBlockId().ToString(), *Definition->ResultItemId.ToString(),
			Definition->ResultQuantity, RemainingIntegrity);
		if (RemainingIntegrity <= 0 && bDestroyWhenDepleted)
		{
			Destroy();
		}
	}
	return true;
}

void AWorldBlockActor::ApplyDungeonBlockPresetData(FName InBlockId, int32 InRemainingIntegrity,
	const TArray<FWorldBlockInteractionDefinition>& InInteractions)
{
	BlockId = InBlockId;
	RemainingIntegrity = FMath::Max(1, InRemainingIntegrity);
	Interactions = InInteractions;
}
