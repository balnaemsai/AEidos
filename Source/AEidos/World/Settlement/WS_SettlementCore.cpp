#include "World/Settlement/WS_SettlementCore.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Save/SaveGameSchema.h"
#include "World/Settlement/SettlementCoreActor.h"
#include "World/Settlement/WS_SettlementSpace.h"

const FName UWS_SettlementCore::KEY_Location(TEXT("SettlementCore.Location"));
const FName UWS_SettlementCore::KEY_Health(TEXT("SettlementCore.Health"));
const FName UWS_SettlementCore::KEY_MaxHealth(TEXT("SettlementCore.MaxHealth"));
const FName UWS_SettlementCore::KEY_Destroyed(TEXT("SettlementCore.Destroyed"));

void UWS_SettlementCore::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	SettlementCoreClass = ASettlementCoreActor::StaticClass();
}

void UWS_SettlementCore::Deinitialize()
{
	ActiveCore.Reset();
	Super::Deinitialize();
}

void UWS_SettlementCore::WriteToSnapshot_Implementation(FEidosWorldSnapshot& OutSnapshot) const
{
	const ASettlementCoreActor* Core = ActiveCore.Get();
	const FVector Location = Core ? Core->GetActorLocation() : SavedLocation;
	const float Health = Core ? Core->GetHealth() : SavedHealth;
	const float MaxHealth = Core ? Core->GetMaxHealth() : SavedMaxHealth;
	const bool bDestroyed = Core ? Core->IsDestroyed() : bSavedDestroyed;

	OutSnapshot.KV.Add(KEY_Location, Location.ToString());
	OutSnapshot.KV.Add(KEY_Health, FString::SanitizeFloat(Health));
	OutSnapshot.KV.Add(KEY_MaxHealth, FString::SanitizeFloat(MaxHealth));
	OutSnapshot.KV.Add(KEY_Destroyed, bDestroyed ? TEXT("1") : TEXT("0"));
}

void UWS_SettlementCore::ApplySnapshot_Implementation(const FEidosWorldSnapshot& Snapshot)
{
	bHasSavedLocation = false;
	SavedLocation = FVector::ZeroVector;
	SavedHealth = 500.f;
	SavedMaxHealth = 500.f;
	bSavedDestroyed = false;

	if (const FString* Location = Snapshot.KV.Find(KEY_Location))
	{
		bHasSavedLocation = SavedLocation.InitFromString(*Location);
	}
	if (const FString* Health = Snapshot.KV.Find(KEY_Health))
	{
		SavedHealth = FMath::Max(0.f, FCString::Atof(**Health));
	}
	if (const FString* MaxHealth = Snapshot.KV.Find(KEY_MaxHealth))
	{
		SavedMaxHealth = FMath::Max(1.f, FCString::Atof(**MaxHealth));
	}
	if (const FString* Destroyed = Snapshot.KV.Find(KEY_Destroyed))
	{
		bSavedDestroyed = (*Destroyed == TEXT("1"));
	}
}

ASettlementCoreActor* UWS_SettlementCore::EnsureSettlementCore()
{
	if (ASettlementCoreActor* Existing = FindExistingCore())
	{
		ActiveCore = Existing;
		Existing->RestoreCoreState(SavedHealth, SavedMaxHealth, bSavedDestroyed);
		return Existing;
	}

	UWorld* World = GetWorld();
	if (!World || !SettlementCoreClass)
	{
		return nullptr;
	}

	const FVector SpawnLocation = bHasSavedLocation ? SavedLocation : ResolveDefaultCoreLocation();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	ASettlementCoreActor* Core = World->SpawnActor<ASettlementCoreActor>(SettlementCoreClass, SpawnLocation, FRotator::ZeroRotator, Params);
	if (!Core)
	{
		UE_LOG(LogTemp, Error, TEXT("[SettlementCore] Failed to spawn settlement core."));
		return nullptr;
	}

	Core->RestoreCoreState(SavedHealth, SavedMaxHealth, bSavedDestroyed);
	ActiveCore = Core;
	SavedLocation = Core->GetActorLocation();
	bHasSavedLocation = true;
	UE_LOG(LogTemp, Log, TEXT("[SettlementCore] Spawned at %s (saved=%d)"), *SavedLocation.ToString(), bHasSavedLocation);
	return Core;
}

FVector UWS_SettlementCore::ResolveDefaultCoreLocation() const
{
	const UWorld* World = GetWorld();
	const UWS_SettlementSpace* SettlementSpace = World ? World->GetSubsystem<UWS_SettlementSpace>() : nullptr;
	if (!SettlementSpace)
	{
		return FVector(0.f, 0.f, CoreElevationCm);
	}

	const TArray<FIntPoint> OwnedChunks = SettlementSpace->GetOwnedChunks();
	if (OwnedChunks.IsEmpty())
	{
		return FVector(0.f, 0.f, CoreElevationCm);
	}

	FVector Accumulated = FVector::ZeroVector;
	for (const FIntPoint& Coord : OwnedChunks)
	{
		Accumulated += SettlementSpace->GetChunkWorldLocation(Coord);
	}
	Accumulated /= static_cast<float>(OwnedChunks.Num());
	Accumulated.Z += CoreElevationCm;
	return Accumulated;
}

ASettlementCoreActor* UWS_SettlementCore::FindExistingCore() const
{
	if (ASettlementCoreActor* Core = ActiveCore.Get())
	{
		return Core;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<ASettlementCoreActor> It(World); It; ++It)
	{
		if (ASettlementCoreActor* Core = *It; IsValid(Core))
		{
			return Core;
		}
	}
	return nullptr;
}
