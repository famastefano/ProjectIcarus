// Stefano Famà (famastefano@gmail.com)

#include "DamageFalloffCurve.h"

#include "LogWeaponSystem.h"

#include "Logging/StructuredLog.h"

template <>
struct TLess<FDamageFalloffKeypoint>
{
	FORCEINLINE bool operator()(const FDamageFalloffKeypoint& A, const FDamageFalloffKeypoint& B) const
	{
		return A.Distance < B.Distance;
	}
};

float UDamageFalloffCurve::GetDamageMultiplier(const FDamageFalloffKeypoint& LowerBound,
                                               const FDamageFalloffKeypoint& UpperBound,
                                               float Distance)
{
	// To use FMath::Lerp we need an Alpha value in the range [0.0, 1.0]
	// Given L = LowerBound, U = UpperBound, D = Distance
	// Our Alpha will be how D is near to U.
	// To map [L, U] into [0.0, 1.0], we just shift the range to [D-L, U-L] and divide by U-L
	const float ShiftedLowerBound = Distance - LowerBound.Distance;
	const float ShiftedUpperBound = UpperBound.Distance - LowerBound.Distance;
	const float Alpha = ShiftedLowerBound / ShiftedUpperBound;
	return FMath::Lerp(LowerBound.DamageMultiplier, UpperBound.DamageMultiplier, Alpha);
}

bool UDamageFalloffCurve::IsValid() const
{
	return !IsSortingRequired();
}

bool UDamageFalloffCurve::IsSortingRequired() const
{
	return !Algo::IsSorted(KeyPoints, TLess<FDamageFalloffKeypoint>{});
}

void UDamageFalloffCurve::AddKeyPoint(FDamageFalloffKeypoint Keypoint)
{
	LastLowerBoundIndex = LastUpperBoundIndex = -1;
	KeyPoints.Add(Keypoint);
	if (KeyPoints.Num() > 1 && TLess<FDamageFalloffKeypoint>{}(KeyPoints.Last(0), KeyPoints.Last(1)))
	{
		SortKeyPoints();
	}
#if WITH_EDITORONLY_DATA
	else
	{
		(void)OnKeyPointsChanged.ExecuteIfBound();
	}
#endif
}

void UDamageFalloffCurve::SortKeyPoints()
{
	LastLowerBoundIndex = LastUpperBoundIndex = -1;
	KeyPoints.Sort(TLess<FDamageFalloffKeypoint>{});

#if WITH_EDITORONLY_DATA
	(void)OnKeyPointsChanged.ExecuteIfBound();
#endif
}

float UDamageFalloffCurve::GetDamageMultiplier(float DistanceInUnits)
{
	check(IsValid());
	if (LastLowerBoundIndex != INDEX_NONE)
	{
		const auto& LowerBound = KeyPoints[LastLowerBoundIndex];
		const auto& UpperBound = KeyPoints[LastUpperBoundIndex];
		if (LowerBound.Distance <= DistanceInUnits)
		{
			return GetDamageMultiplier(LowerBound, UpperBound, DistanceInUnits);
		}
	}

	LastLowerBoundIndex = KeyPoints.FindLastByPredicate([DistanceInUnits](const FDamageFalloffKeypoint& A)
	{
		return A.Distance <= DistanceInUnits;
	});
	if (LastLowerBoundIndex != INDEX_NONE)
	{
		if (LastLowerBoundIndex < KeyPoints.Num() - 1)
		{
			LastUpperBoundIndex = LastLowerBoundIndex + 1;
			return GetDamageMultiplier(KeyPoints[LastLowerBoundIndex], KeyPoints[LastUpperBoundIndex], DistanceInUnits);
		}

		LastUpperBoundIndex = LastLowerBoundIndex;
		return KeyPoints[LastLowerBoundIndex].DamageMultiplier;
	}

#if !UE_BUILD_SHIPPING
	UE_LOGFMT(LogWeaponSystem, Warning,
	          "FDamageFalloffCurve::GetScaledFactor({Distance}) doesn't have a KeyPoint <= than {Distance}, so it returned zero.",
	          ("Distance", DistanceInUnits));
#endif

	return 0;
}
