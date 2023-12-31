// Stefano Famà (famastefano@gmail.com)

#include "DamageFalloffCurve.h"

#include "LogWeaponSystem.h"

#include "Logging/StructuredLog.h"

template <>
struct TLess<FDamageFalloffKeypoint>
{
	FORCEINLINE bool operator()(const FDamageFalloffKeypoint& A, const FDamageFalloffKeypoint& B) const
	{
		return A.DistanceInUnits < B.DistanceInUnits;
	}
};

float FDamageFalloffCurve::GetScaledFactor(const FDamageFalloffKeypoint& LowerBound,
                                           const FDamageFalloffKeypoint& UpperBound,
                                           float Distance)
{
	// To use FMath::Lerp we need an Alpha value in the range [0.0, 1.0]
	// Given L = LowerBound, U = UpperBound, D = Distance
	// Our Alpha will be how D is near to U.
	// To map [L, U] into [0.0, 1.0], we just shift the range to [D-L, U-L] and divide by U-L
	const float ShiftedLowerBound = Distance - LowerBound.DistanceInUnits;
	const float ShiftedUpperBound = UpperBound.DistanceInUnits - LowerBound.DistanceInUnits;
	const float Alpha = ShiftedLowerBound / ShiftedUpperBound;
	return FMath::Lerp(LowerBound.DamageScaling, UpperBound.DamageScaling, Alpha);
}

bool FDamageFalloffCurve::IsValid() const
{
	return !IsSortingRequired();
}

bool FDamageFalloffCurve::IsSortingRequired() const
{
	return !Algo::IsSorted(KeyPoints, TLess<FDamageFalloffKeypoint>{});
}

void FDamageFalloffCurve::AddKeyPoint(FDamageFalloffKeypoint Keypoint)
{
	LastLowerBoundIndex = LastUpperBoundIndex = -1;
	KeyPoints.Add(Keypoint);
	if (KeyPoints.Num() > 1 && TLess<FDamageFalloffKeypoint>{}(KeyPoints.Last(0), KeyPoints.Last(1)))
	{
		SortKeyPoints();
	}
}

void FDamageFalloffCurve::SortKeyPoints()
{
	LastLowerBoundIndex = LastUpperBoundIndex = -1;
	KeyPoints.Sort(TLess<FDamageFalloffKeypoint>{});
}

float FDamageFalloffCurve::GetScaledFactor(float DistanceInUnits)
{
	check(IsValid());
	if (LastLowerBoundIndex != INDEX_NONE)
	{
		const auto& LowerBound = KeyPoints[LastLowerBoundIndex];
		const auto& UpperBound = KeyPoints[LastUpperBoundIndex];
		if (LowerBound.DistanceInUnits <= DistanceInUnits)
		{
			return GetScaledFactor(LowerBound, UpperBound, DistanceInUnits);
		}
	}

	LastLowerBoundIndex = KeyPoints.FindLastByPredicate([DistanceInUnits](const FDamageFalloffKeypoint& A)
	{
		return A.DistanceInUnits <= DistanceInUnits;
	});
	if (LastLowerBoundIndex != INDEX_NONE)
	{
		if (LastLowerBoundIndex < KeyPoints.Num() - 1)
		{
			LastUpperBoundIndex = LastLowerBoundIndex + 1;
			return GetScaledFactor(KeyPoints[LastLowerBoundIndex], KeyPoints[LastUpperBoundIndex], DistanceInUnits);
		}

		LastUpperBoundIndex = LastLowerBoundIndex;
		return KeyPoints[LastLowerBoundIndex].DamageScaling;
	}

#if !UE_BUILD_SHIPPING
	UE_LOGFMT(LogWeaponSystem, Warning,
	          "FDamageFalloffCurve::GetScaledFactor({Distance}) doesn't have a KeyPoint <= than {Distance}, so it returned zero.",
	          ("Distance", DistanceInUnits));
#endif

	return 0;
}
