// Stefano Famà (famastefano@gmail.com)

#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"

class UTestWorldSubsystem;

class TESTWORLD_API FTestWorldHelper
{
	UTestWorldSubsystem* Subsystem;
	UWorld* World;
	bool IsSharedWorld;
	decltype(GFrameCounter) OldGFrameCounter;

public:
	explicit FTestWorldHelper() :
		Subsystem(nullptr),
		World(nullptr),
		IsSharedWorld(false),
		OldGFrameCounter(GFrameCounter)
	{
	}

	explicit FTestWorldHelper(UTestWorldSubsystem* Subsystem,
	                                UWorld* World,
	                                bool IsSharedWorld);

	FTestWorldHelper(const FTestWorldHelper&) = delete;
	FTestWorldHelper& operator=(const FTestWorldHelper&) = delete;

	FTestWorldHelper(FTestWorldHelper&& Other) noexcept;
	FTestWorldHelper& operator=(FTestWorldHelper&& Other) noexcept;

	~FTestWorldHelper();

	FORCEINLINE UWorld* operator->() const
	{
		check(World);
		return World;
	}

	void Tick(float DeltaTime = 0.001953125) const;
	void TickUntil(float DeltaTime, const TUniqueFunction<bool()>& ShouldStopTicking) const;
	void TickWithVariableDeltaTimeUntil(const TUniqueFunction<float(float)>& CalculateNextDeltaTime,
	                                    const TUniqueFunction<bool()>& ShouldStopTicking) const;
};
