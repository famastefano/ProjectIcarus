﻿// Stefano Famà (famastefano@gmail.com)

#pragma once

#include "CoreMinimal.h"
#include "DamageFalloffCurve.generated.h"

USTRUCT(BlueprintType)
struct FDamageFalloffKeypoint
{
	GENERATED_BODY()

	float DamageScaling;
	float DistanceInUnits;
};

USTRUCT(Blueprintable, BlueprintType)
struct WEAPONSYSTEMRUNTIME_API FDamageFalloffCurve
{
	GENERATED_BODY()

	UPROPERTY(EditInstanceOnly, Category="Damage")
	TArray<FDamageFalloffKeypoint> KeyPoints;

	decltype(KeyPoints)::SizeType LowerBoundIndex = INDEX_NONE;
	decltype(KeyPoints)::SizeType UpperBoundIndex = INDEX_NONE;

	static int CompareKeyPoints(const FDamageFalloffKeypoint& A, const FDamageFalloffKeypoint& B);
	static float GetScaledFactor(const FDamageFalloffKeypoint& LowerBound,
	                             const FDamageFalloffKeypoint& UpperBound,
	                             float Distance);

	bool IsValid() const;
	bool IsSortingRequired() const;
	void AddKeyPoint(FDamageFalloffKeypoint Keypoint);
	void SortKeyPoints();
	float GetScaledFactor(float DistanceInUnits);
};