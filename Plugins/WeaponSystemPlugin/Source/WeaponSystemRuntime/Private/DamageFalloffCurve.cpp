// Stefano Famà (famastefano@gmail.com)

#include "DamageFalloffCurve.h"

#include "LogWeaponSystem.h"

#include "Logging/StructuredLog.h"

int FDamageFalloffCurve::CompareKeyPoints(const FDamageFalloffKeypoint& A, const FDamageFalloffKeypoint& B)
{
	if (FMath::IsNearlyEqual(A.DistanceInUnits, B.DistanceInUnits, 0.5))
	{
		return 0;
	}
	return B.DistanceInUnits - A.DistanceInUnits;
}

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
	for (decltype(KeyPoints)::SizeType i = 0; i < KeyPoints.Num() - 1; ++i)
	{
		const auto& A = KeyPoints[i];
		const auto& B = KeyPoints[i + 1];
		if (CompareKeyPoints(A, B) > 0)
		{
			return true;
		}
	}
	return false;
}

void FDamageFalloffCurve::AddKeyPoint(FDamageFalloffKeypoint Keypoint)
{
	LowerBoundIndex = UpperBoundIndex = -1;
	KeyPoints.Add(Keypoint);
	if (KeyPoints.Num() > 1 && CompareKeyPoints(KeyPoints.Last(1), KeyPoints.Last(0)) > 0)
	{
		SortKeyPoints();
	}
}

void FDamageFalloffCurve::SortKeyPoints()
{
	LowerBoundIndex = UpperBoundIndex = -1;
	Algo::Sort(KeyPoints, [](const FDamageFalloffKeypoint& A, const FDamageFalloffKeypoint& B)
	{
		return CompareKeyPoints(A, B) <= 0;
	});
}

float FDamageFalloffCurve::GetScaledFactor(float DistanceInUnits)
{
	check(IsValid());
	if (LowerBoundIndex != INDEX_NONE)
	{
		const auto& LowerBound = KeyPoints[LowerBoundIndex];
		const auto& UpperBound = KeyPoints[UpperBoundIndex];
		if (LowerBound.DistanceInUnits <= DistanceInUnits)
		{
			return GetScaledFactor(LowerBound, UpperBound, DistanceInUnits);
		}
	}

	LowerBoundIndex = KeyPoints.FindLastByPredicate([DistanceInUnits](const FDamageFalloffKeypoint& A)
	{
		return A.DistanceInUnits <= DistanceInUnits;
	});
	if (LowerBoundIndex != INDEX_NONE)
	{
		if (LowerBoundIndex < KeyPoints.Num() - 1)
		{
			UpperBoundIndex = LowerBoundIndex + 1;
			return GetScaledFactor(KeyPoints[LowerBoundIndex], KeyPoints[UpperBoundIndex], DistanceInUnits);
		}

		UpperBoundIndex = LowerBoundIndex;
		return KeyPoints[LowerBoundIndex].DamageScaling;
	}

#if !UE_BUILD_SHIPPING
	UE_LOGFMT(LogWeaponSystem, Warning,
	          "FDamageFalloffCurve::GetScaledFactor({Distance}) doesn't have a KeyPoint <= than {Distance}, so it returned zero.",
	          DistanceInUnits);
#endif

	return 0;
}
