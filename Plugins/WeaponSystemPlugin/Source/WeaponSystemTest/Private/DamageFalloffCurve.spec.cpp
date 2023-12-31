#include "LogWeaponSystemTest.h"
#include "DamageFalloffCurve.h"

#include "Logging/StructuredLog.h"
#include "Misc/AutomationTest.h"

BEGIN_DEFINE_SPEC(FDamageFalloffCurve_Spec, "WeaponSystemPlugin.Runtime.DamageFalloffCurve",
                  EAutomationTestFlags::ApplicationContextMask
                  | EAutomationTestFlags::HighPriority | EAutomationTestFlags::ProductFilter)

	FDamageFalloffCurve Curve;

END_DEFINE_SPEC(FDamageFalloffCurve_Spec)

void FDamageFalloffCurve_Spec::Define()
{
	Describe("A Damage Falloff Curve", [this]
	{
		BeforeEach([this]
		{
			Curve = {};
		});

		Describe("If empty", [this]
		{
			It("Should be valid", [this]
			{
				TestTrueExpr(Curve.IsValid());
			});

			It("Should return a scaling factor", [this]
			{
				TestTrueExpr(!FMath::IsNaN(Curve.GetScaledFactor(0)));
			});

			It("Shoudlnt require to be sorted", [this]
			{
				TestTrueExpr(!Curve.IsSortingRequired());
			});

			It("Shouldnt raise errors when sorted", [this]
			{
				Curve.SortKeyPoints();
			});
		});

		Describe("If it has keypoints", [this]
		{
			It("Shouldnt require sorting, if keypoints are added via the class methods", [this]
			{
				TArray KeyPointsToInsert
				{
					FDamageFalloffKeypoint{.DamageScaling = 1, .DistanceInUnits = 1},
					FDamageFalloffKeypoint{.DamageScaling = 2, .DistanceInUnits = 2},
					FDamageFalloffKeypoint{.DamageScaling = 3, .DistanceInUnits = 3},
					FDamageFalloffKeypoint{.DamageScaling = 4, .DistanceInUnits = 4},
				};
				auto begin = KeyPointsToInsert.rbegin();
				auto end = KeyPointsToInsert.rend();
				while (begin != end)
				{
					Curve.AddKeyPoint(*begin);
					TestTrueExpr(!Curve.IsSortingRequired());
					++begin;
				}
			});

			It("Shouldnt require sorting, if keypoints are inserted manually in the correct order", [this]
			{
				TArray KeyPointsToInsert
				{
					FDamageFalloffKeypoint{.DamageScaling = 1, .DistanceInUnits = 1},
					FDamageFalloffKeypoint{.DamageScaling = 2, .DistanceInUnits = 2},
					FDamageFalloffKeypoint{.DamageScaling = 3, .DistanceInUnits = 3},
					FDamageFalloffKeypoint{.DamageScaling = 4, .DistanceInUnits = 4},
				};
				Curve.KeyPoints.Append(KeyPointsToInsert);
				TestTrueExpr(!Curve.IsSortingRequired());
			});

			It("Should require sorting, if keypoints are inserted manually but without caring about the order", [this]
			{
				TArray KeyPointsToInsert
				{
					FDamageFalloffKeypoint{.DamageScaling = 4, .DistanceInUnits = 4},
					FDamageFalloffKeypoint{.DamageScaling = 2, .DistanceInUnits = 2},
					FDamageFalloffKeypoint{.DamageScaling = 1, .DistanceInUnits = 1},
					FDamageFalloffKeypoint{.DamageScaling = 3, .DistanceInUnits = 3},
				};
				Curve.KeyPoints.Append(KeyPointsToInsert);
				TestTrueExpr(Curve.IsSortingRequired());
			});
		});

		// TODO: Sorting works correctly
		// TODO: Acquiring scaling factor works as expected
	});
}
