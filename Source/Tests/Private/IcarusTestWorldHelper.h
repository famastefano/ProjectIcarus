// Stefano Famà (famastefano@gmail.com)

#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"

class UIcarusTestSubsystem;

class TESTS_API FIcarusTestWorldHelper
{
	UIcarusTestSubsystem* Subsystem;
	UWorld* World;
	bool IsSharedWorld;
	decltype(GFrameCounter) OldGFrameCounter;

public:
	explicit FIcarusTestWorldHelper() :
		Subsystem(nullptr),
		World(nullptr),
		IsSharedWorld(false),
		OldGFrameCounter(GFrameCounter)
	{
	}

	explicit FIcarusTestWorldHelper(UIcarusTestSubsystem* Subsystem,
	                                UWorld* World,
	                                bool IsSharedWorld);

	FIcarusTestWorldHelper(FIcarusTestWorldHelper const&) = delete;
	FIcarusTestWorldHelper& operator=(FIcarusTestWorldHelper const&) = delete;

	FIcarusTestWorldHelper(FIcarusTestWorldHelper&& Other) noexcept;
	FIcarusTestWorldHelper& operator=(FIcarusTestWorldHelper&& Other) noexcept;

	~FIcarusTestWorldHelper();

	FORCEINLINE UWorld* operator->() const
	{
		check(World);
		return World;
	}

	void Tick(float DeltaTime = 0.001953125) const;
	void TickUntil(float DeltaTime, TUniqueFunction<bool()> const& ShouldStopTicking) const;
	void TickWithVariableDeltaTimeUntil(TUniqueFunction<float(float)> const& CalculateNextDeltaTime,
	                                    TUniqueFunction<bool()> const& ShouldStopTicking) const;
};
