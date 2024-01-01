// Stefano Famà (famastefano@gmail.com)

#pragma once

#include "CoreMinimal.h"
#include "DamageFalloffCurve.generated.h"

#if WITH_EDITORONLY_DATA
DECLARE_DELEGATE(FDamageFalloffCurveKeyPointsChanged)
#endif

USTRUCT(BlueprintType)
struct FDamageFalloffKeypoint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, meta=(UIMin=0, ClampMin=0, Units="Percent"))
	float DamageMultiplier;

	UPROPERTY(EditAnywhere, meta=(UIMin=0, ClampMin=0, Units="Meters"))
	float Distance;
};

UCLASS(Blueprintable, BlueprintType)
class WEAPONSYSTEMRUNTIME_API UDamageFalloffCurve : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditInstanceOnly, Category="Damage")
	TArray<FDamageFalloffKeypoint> KeyPoints;

#if WITH_EDITORONLY_DATA
	FDamageFalloffCurveKeyPointsChanged OnKeyPointsChanged;
#endif

private:
	decltype(KeyPoints)::SizeType LastLowerBoundIndex = INDEX_NONE;
	decltype(KeyPoints)::SizeType LastUpperBoundIndex = INDEX_NONE;

public:
	static float GetDamageMultiplier(const FDamageFalloffKeypoint& LowerBound,
	                                 const FDamageFalloffKeypoint& UpperBound,
	                                 float Distance);

	bool IsValid() const;
	bool IsSortingRequired() const;
	void AddKeyPoint(FDamageFalloffKeypoint Keypoint);

	UFUNCTION(BlueprintCallable, CallInEditor)
	void SortKeyPoints();

	float GetDamageMultiplier(float DistanceInUnits);
};
