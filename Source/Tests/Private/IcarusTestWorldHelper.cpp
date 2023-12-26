// Stefano Famà (famastefano@gmail.com)


#include "IcarusTestWorldHelper.h"

#include "IcarusTestSubsystem.h"
#include "LogIcarusTests.h"

#include "Engine/CoreSettings.h"

#include "HAL/ThreadManager.h"

#include "Logging/StructuredLog.h"

FIcarusTestWorldHelper::FIcarusTestWorldHelper(UIcarusTestSubsystem* Subsystem,
                                               UWorld* World,
                                               bool IsSharedWorld)
	: Subsystem(Subsystem), World(World), IsSharedWorld(IsSharedWorld), OldGFrameCounter(GFrameCounter)
{
}

FIcarusTestWorldHelper::FIcarusTestWorldHelper(FIcarusTestWorldHelper&& Other) noexcept : Subsystem(Other.Subsystem), OldGFrameCounter(Other.OldGFrameCounter)
{
	World = std::exchange(Other.World, nullptr);
	IsSharedWorld = std::exchange(Other.IsSharedWorld, false);
}

FIcarusTestWorldHelper& FIcarusTestWorldHelper::operator=(FIcarusTestWorldHelper&& Other) noexcept
{
	World = std::exchange(Other.World, nullptr);
	IsSharedWorld = std::exchange(Other.IsSharedWorld, false);
	OldGFrameCounter = Other.OldGFrameCounter;
	return *this;
}

FIcarusTestWorldHelper::~FIcarusTestWorldHelper()
{
	if (World && !IsSharedWorld)
	{
		GFrameCounter = OldGFrameCounter;
		Subsystem->DestroyPrivateWorld(World->GetFName());
	}
}

void FIcarusTestWorldHelper::Tick(float DeltaTime) const
{
	check(IsInGameThread());
	check(World);
	UE_LOGFMT(LogIcarusTests, Log, "Ticking {Name} by {Seconds} s.", World->GetFName(), DeltaTime);
	StaticTick(DeltaTime, !!GAsyncLoadingUseFullTimeLimit, GAsyncLoadingTimeLimit / 1000.f);
	World->Tick(LEVELTICK_All, DeltaTime);
	FTickableGameObject::TickObjects(nullptr, LEVELTICK_All, false, DeltaTime);
	GFrameCounter++;
	FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);
	FThreadManager::Get().Tick();
	FTSTicker::GetCoreTicker().Tick(DeltaTime);
	GEngine->TickDeferredCommands();
}

void FIcarusTestWorldHelper::TickUntil(float DeltaTime, TUniqueFunction<bool()> const& ShouldStopTicking) const
{
	check(ShouldStopTicking);

	while (!ShouldStopTicking())
		Tick(DeltaTime);
}

void FIcarusTestWorldHelper::TickWithVariableDeltaTimeUntil(TUniqueFunction<float(float)> const& CalculateNextDeltaTime,
                                                            TUniqueFunction<bool()> const& ShouldStopTicking) const
{
	check(CalculateNextDeltaTime);
	check(ShouldStopTicking);

	float DeltaTime = CalculateNextDeltaTime(0);
	while (!ShouldStopTicking())
	{
		DeltaTime = CalculateNextDeltaTime(DeltaTime);
		Tick(DeltaTime);
	}
}
