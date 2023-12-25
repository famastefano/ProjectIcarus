// Stefano Famà (famastefano@gmail.com)


#include "IcarusTestWorldHelper.h"

#include "IcarusTestSubsystem.h"

#include "HAL/ThreadManager.h"

FIcarusTestWorldHelper::FIcarusTestWorldHelper(UIcarusTestSubsystem* Subsystem,
                                               UWorld* World,
                                               FName WorldName,
                                               bool IsSharedWorld)
	: Subsystem(Subsystem), World(World), WorldName(WorldName), IsSharedWorld(IsSharedWorld)
{
}

FIcarusTestWorldHelper::FIcarusTestWorldHelper(FIcarusTestWorldHelper&& Other) noexcept : Subsystem(Other.Subsystem)
{
	World = std::exchange(Other.World, nullptr);
	WorldName = std::exchange(Other.WorldName, NAME_None);
	IsSharedWorld = std::exchange(Other.IsSharedWorld, false);
}

FIcarusTestWorldHelper& FIcarusTestWorldHelper::operator=(FIcarusTestWorldHelper&& Other) noexcept
{
	World = std::exchange(Other.World, nullptr);
	IsSharedWorld = std::exchange(Other.IsSharedWorld, false);
	return *this;
}

FIcarusTestWorldHelper::~FIcarusTestWorldHelper()
{
	if (World && !IsSharedWorld)
		Subsystem->DestroyPrivateWorld(WorldName);
}

void FIcarusTestWorldHelper::Tick(float DeltaTime) const
{
	StaticTick(DeltaTime);
	World->Tick(LEVELTICK_All, DeltaTime);
	FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);
	FTSTicker::GetCoreTicker().Tick(FApp::GetDeltaTime());
	FThreadManager::Get().Tick();
	GEngine->TickDeferredCommands();
}

void FIcarusTestWorldHelper::TickUntil(float DeltaTime, TUniqueFunction<bool()> const& ShouldStopTicking) const
{
	check(World);
	check(ShouldStopTicking);

	while (!ShouldStopTicking())
		Tick(DeltaTime);
}

void FIcarusTestWorldHelper::TickWithVariableDeltaTimeUntil(TUniqueFunction<float(float)> const& CalculateNextDeltaTime,
                                                            TUniqueFunction<bool()> const& ShouldStopTicking) const
{
	check(World);
	check(CalculateNextDeltaTime);
	check(ShouldStopTicking);

	float DeltaTime = CalculateNextDeltaTime(0);
	while (!ShouldStopTicking())
	{
		DeltaTime = CalculateNextDeltaTime(DeltaTime);
		Tick(DeltaTime);
	}
}
