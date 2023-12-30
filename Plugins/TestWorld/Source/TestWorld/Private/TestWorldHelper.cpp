// Stefano Famà (famastefano@gmail.com)


#include "TestWorldHelper.h"

#include "TestWorldSubsystem.h"
#include "LogTestWorld.h"

#include "Engine/CoreSettings.h"

#include "HAL/ThreadManager.h"

#include "Logging/StructuredLog.h"

FTestWorldHelper::FTestWorldHelper(UTestWorldSubsystem* Subsystem,
                                               UWorld* World,
                                               bool IsSharedWorld)
	: Subsystem(Subsystem), World(World), IsSharedWorld(IsSharedWorld), OldGFrameCounter(GFrameCounter)
{
}

FTestWorldHelper::FTestWorldHelper(FTestWorldHelper&& Other) noexcept : Subsystem(Other.Subsystem),
	OldGFrameCounter(Other.OldGFrameCounter)
{
	World = std::exchange(Other.World, nullptr);
	IsSharedWorld = std::exchange(Other.IsSharedWorld, false);
}

FTestWorldHelper& FTestWorldHelper::operator=(FTestWorldHelper&& Other) noexcept
{
	World = std::exchange(Other.World, nullptr);
	IsSharedWorld = std::exchange(Other.IsSharedWorld, false);
	OldGFrameCounter = Other.OldGFrameCounter;
	return *this;
}

FTestWorldHelper::~FTestWorldHelper()
{
	if (World && !IsSharedWorld)
	{
		GFrameCounter = OldGFrameCounter;
		Subsystem->DestroyPrivateWorld(World->GetFName());
	}
}

void FTestWorldHelper::Tick(float DeltaTime) const
{
	check(IsInGameThread());
	check(World);
	UE_LOGFMT(LogTestWorld, Log, "Ticking {Name} by {Seconds} s.", World->GetFName(), DeltaTime);
	StaticTick(DeltaTime, !!GAsyncLoadingUseFullTimeLimit, GAsyncLoadingTimeLimit / 1000.f);
	World->Tick(LEVELTICK_All, DeltaTime);
	FTickableGameObject::TickObjects(nullptr, LEVELTICK_All, false, DeltaTime);
	GFrameCounter++;
	FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);
	FThreadManager::Get().Tick();
	FTSTicker::GetCoreTicker().Tick(DeltaTime);
	GEngine->TickDeferredCommands();
}

void FTestWorldHelper::TickUntil(float DeltaTime, const TUniqueFunction<bool()>& ShouldStopTicking) const
{
	check(ShouldStopTicking);

	while (!ShouldStopTicking())
	{
		Tick(DeltaTime);
	}
}

void FTestWorldHelper::TickWithVariableDeltaTimeUntil(const TUniqueFunction<float(float)>& CalculateNextDeltaTime,
                                                            const TUniqueFunction<bool()>& ShouldStopTicking) const
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
