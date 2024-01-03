// Stefano Famà (famastefano@gmail.com)


#include "TestWorldSubsystem.h"

#include "TestWorldGameInstance.h"
#include "LogTestWorld.h"

#include "Logging/StructuredLog.h"

FTestWorldData UTestWorldSubsystem::MakeTestWorld(FName Name)
{
	check(IsInGameThread());

	UE_LOGFMT(LogTestWorld, Log, "Creating test world {Name}.", Name);

	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, true, Name);
	UTestWorldGameInstance* GameInstance = NewObject<UTestWorldGameInstance>();
	GameInstance->AddToRoot();

	WorldContext.SetCurrentWorld(World);
	World->UpdateWorldComponents(true, true);
	World->AddToRoot();
	World->SetFlags(RF_Public | RF_Standalone);
	World->SetShouldTick(false);

	UE_LOGFMT(LogTestWorld, Log, "Test world {Name} created.", Name);

	GameInstance->InitForTest(*World);

#if WITH_EDITOR
	GEngine->BroadcastLevelActorListChanged();
#endif

	World->InitializeActorsForPlay(FURL());
	auto* Settings = World->GetWorldSettings();
	Settings->MinUndilatedFrameTime = 0.0001;
	Settings->MaxUndilatedFrameTime = 10;
	World->BeginPlay();

	UE_LOGFMT(LogTestWorld, Log, "Test world {Name} begin play completed.", Name);

	return {GameInstance, World};
}

void UTestWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	UE_LOGFMT(LogTestWorld, Log, "Initializing Test Subsystem...");
	Super::Initialize(Collection);
}

void UTestWorldSubsystem::Deinitialize()
{
	UE_LOGFMT(LogTestWorld, Log, "Deinitializing Test Subsystem.");

	if (!PrivateWorlds.IsEmpty())
	{
		for (const auto& [Name, Env] : PrivateWorlds)
		{
			UE_LOGFMT(LogTestWorld, Warning, "World {Name} wasn't disposed during test cleanup.", Name);
			Env.World->DestroyWorld(true);
			Env.GameInstance->RemoveFromRoot();
		}
		PrivateWorlds.Empty();
	}

	if (SharedWorld.World)
	{
		SharedWorld.World->DestroyWorld(true);
		SharedWorld.GameInstance->RemoveFromRoot();
	}

	Super::Deinitialize();
}

FTestWorldHelper UTestWorldSubsystem::GetPrivateWorld(FName Name)
{
	check(IsInGameThread());
	checkf(PrivateWorlds.Find(Name) == nullptr, TEXT("This test world has already been created"));

	const auto& [GameInstance, World] = PrivateWorlds.Add(Name, MakeTestWorld(Name));
	return FTestWorldHelper{this, World, false};
}

FTestWorldHelper UTestWorldSubsystem::GetSharedWorld()
{
	if (!SharedWorld.World)
	{
		SharedWorld = MakeTestWorld("TestWorld_SharedWorld");
	}

	FTestWorldHelper Helper{this, SharedWorld.World, true};
	return Helper;
}

void UTestWorldSubsystem::DestroyPrivateWorld(FName Name)
{
	const auto& [GameInstance, World] = PrivateWorlds.FindAndRemoveChecked(Name);
	World->DestroyWorld(true);
	GameInstance->RemoveFromRoot();
	UE_LOGFMT(LogTestWorld, Log, "Destroyed test world {Name}.", Name);
}
