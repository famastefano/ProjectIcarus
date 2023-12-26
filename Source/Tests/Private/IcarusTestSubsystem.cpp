// Stefano Famà (famastefano@gmail.com)


#include "IcarusTestSubsystem.h"

#include "IcarusTestGameInstance.h"
#include "LogIcarusTests.h"

#include "Logging/StructuredLog.h"

FTestWorldData UIcarusTestSubsystem::MakeTestWorld(FName Name)
{
	check(IsInGameThread());

	UE_LOGFMT(LogIcarusTests, Log, "Creating test world {Name}.", Name);

	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, true, Name);
	UIcarusTestGameInstance* GameInstance = NewObject<UIcarusTestGameInstance>();
	GameInstance->AddToRoot();

	WorldContext.SetCurrentWorld(World);
	World->UpdateWorldComponents(true, true);
	World->AddToRoot();
	World->SetFlags(RF_Public | RF_Standalone);
	World->SetShouldTick(false);

	UE_LOGFMT(LogIcarusTests, Log, "Test world {Name} created.", Name);

	GameInstance->InitForTest(*World);

	GEngine->BroadcastLevelActorListChanged();

	World->InitializeActorsForPlay(FURL());
	auto* Settings = World->GetWorldSettings();
	Settings->MinUndilatedFrameTime = 0.0001;
	Settings->MaxUndilatedFrameTime = 10;
	World->BeginPlay();

	UE_LOGFMT(LogIcarusTests, Log, "Test world {Name} begin play completed.", Name);

	return {GameInstance, World};
}

void UIcarusTestSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	UE_LOGFMT(LogIcarusTests, Log, "Initializing Test Subsystem...");
	Super::Initialize(Collection);
}

void UIcarusTestSubsystem::Deinitialize()
{
	UE_LOGFMT(LogIcarusTests, Log, "Deinitializing Test Subsystem.");

	if (!PrivateWorlds.IsEmpty())
	{
		for (auto const& [Name, Env] : PrivateWorlds)
		{
			UE_LOGFMT(LogIcarusTests, Warning, "World {Name} wasn't disposed during test cleanup.", Name);
			Env.World->DestroyWorld(true);
			Env.GameInstance->RemoveFromRoot();
		}
		PrivateWorlds.Empty();
	}

	if(SharedWorld.World)
	{
		SharedWorld.World->DestroyWorld(true);
		SharedWorld.GameInstance->RemoveFromRoot();
	}

	Super::Deinitialize();
}

FIcarusTestWorldHelper UIcarusTestSubsystem::GetPrivateWorld(FName Name)
{
	check(IsInGameThread());
	if (const FTestWorldData* Data = PrivateWorlds.Find(Name))
		return FIcarusTestWorldHelper{this, Data->World, false};

	auto const& [GameInstance, World] = PrivateWorlds.Add(Name, MakeTestWorld(Name));
	return FIcarusTestWorldHelper{this, World, false};
}

FIcarusTestWorldHelper UIcarusTestSubsystem::GetSharedWorld()
{
	if (!SharedWorld.World)
		SharedWorld = MakeTestWorld("IcarusTestSharedWorld");

	FIcarusTestWorldHelper Helper{this, SharedWorld.World, true};
	return Helper;
}

void UIcarusTestSubsystem::DestroyPrivateWorld(FName Name)
{
	auto const& [GameInstance, World] = PrivateWorlds.FindAndRemoveChecked(Name);
	World->DestroyWorld(true);
	GameInstance->RemoveFromRoot();
	UE_LOGFMT(LogIcarusTests, Log, "Destroyed test world {Name}.", Name);
}
