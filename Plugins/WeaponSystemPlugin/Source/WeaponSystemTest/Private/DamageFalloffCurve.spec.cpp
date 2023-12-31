#include "LogWeaponSystemTest.h"
#include "DamageFalloffCurve.h"

#include "Algo/Compare.h"

#include "Logging/StructuredLog.h"
#include "Misc/AutomationTest.h"

BEGIN_DEFINE_SPEC(FDamageFalloffCurve_Spec, "WeaponSystemPlugin.Runtime.DamageFalloffCurve",
                  EAutomationTestFlags::ApplicationContextMask
                  | EAutomationTestFlags::HighPriority | EAutomationTestFlags::ProductFilter)

	FDamageFalloffCurve Curve;

END_DEFINE_SPEC(FDamageFalloffCurve_Spec)

void FDamageFalloffCurve_Spec::Define()
{
	constexpr float FactorComparisonTolerance = 0.01;

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

			It("Should sort the element properly", [this]
			{
				TArray KeyPointsToInsert
				{
					FDamageFalloffKeypoint{.DamageScaling = 4, .DistanceInUnits = 4},
					FDamageFalloffKeypoint{.DamageScaling = 2, .DistanceInUnits = 2},
					FDamageFalloffKeypoint{.DamageScaling = 1, .DistanceInUnits = 1},
					FDamageFalloffKeypoint{.DamageScaling = 3, .DistanceInUnits = 3},
				};
				TArray ExpectedKeyPoints
				{
					FDamageFalloffKeypoint{.DamageScaling = 1, .DistanceInUnits = 1},
					FDamageFalloffKeypoint{.DamageScaling = 2, .DistanceInUnits = 2},
					FDamageFalloffKeypoint{.DamageScaling = 3, .DistanceInUnits = 3},
					FDamageFalloffKeypoint{.DamageScaling = 4, .DistanceInUnits = 4},
				};

				Curve.KeyPoints.Append(KeyPointsToInsert);
				Curve.SortKeyPoints();
				TestTrueExpr(!Curve.IsSortingRequired());
				using FKeyPointConst = const FDamageFalloffKeypoint&;
				bool AreExpectedKeyPoints = Algo::Compare(Curve.KeyPoints, ExpectedKeyPoints,
				                                          [](FKeyPointConst A, FKeyPointConst B)
				                                          {
					                                          return std::tie(A.DamageScaling, A.DistanceInUnits) ==
						                                          std::tie(
							                                          B.DamageScaling, B.DistanceInUnits);
				                                          });
				TestTrueExpr(AreExpectedKeyPoints);
			});

			Describe("And you want the scaling factor", [this]
			{
				BeforeEach([this]
				{
					Curve.KeyPoints.Append(
						{
							FDamageFalloffKeypoint{.DamageScaling = 1, .DistanceInUnits = 1},
							FDamageFalloffKeypoint{.DamageScaling = 2, .DistanceInUnits = 2},
							FDamageFalloffKeypoint{.DamageScaling = 3, .DistanceInUnits = 3},
							FDamageFalloffKeypoint{.DamageScaling = 4, .DistanceInUnits = 4}
						}
					);
				});

				It("Should return zero, if Distance < First.Distance", [this]
				{
					const double Distance = Curve.KeyPoints[0].DistanceInUnits - 1;
					TestTrueExpr(Curve.GetScaledFactor(Distance) == 0.0);
				});

				It("Should return First.Factor, if Distance == First.Distance", [this]
				{
					const auto& First = Curve.KeyPoints[0];
					const double Factor = Curve.GetScaledFactor(First.DistanceInUnits);
					TestTrueExpr(FMath::IsNearlyEqual(First.DamageScaling, Factor, FactorComparisonTolerance));
				});

				It("Should return Lower.Factor, if Distance == Lower.Distance", [this]
				{
					const auto& Lower = Curve.KeyPoints[2];
					const double Factor = Curve.GetScaledFactor(Lower.DistanceInUnits);
					TestTrueExpr(FMath::IsNearlyEqual(Lower.DamageScaling, Factor, FactorComparisonTolerance));
				});

				It("Should return Last.Factor, if Distance == Last.Distance", [this]
				{
					const auto& Last = Curve.KeyPoints.Last();
					const double Factor = Curve.GetScaledFactor(Last.DistanceInUnits);
					TestTrueExpr(FMath::IsNearlyEqual(Last.DamageScaling, Factor,FactorComparisonTolerance));
				});


				It("Should return Last.Factor, if Distance > Last.Distance", [this]
				{
					const auto& Last = Curve.KeyPoints.Last();
					const double Factor = Curve.GetScaledFactor(Last.DistanceInUnits + 1);
					TestTrueExpr(FMath::IsNearlyEqual(Last.DamageScaling, Factor, FactorComparisonTolerance));
				});

				It("Should return the expected factor, if Distance is half the one between Lower and Upper", [this]
				{
					const auto& Lower = Curve.KeyPoints[2];
					const auto& Upper = Curve.KeyPoints[3];
					const float ExpectedFactor = (Lower.DamageScaling + Upper.DamageScaling) / 2;
					const float Distance = (Lower.DistanceInUnits + Upper.DistanceInUnits) / 2;
					const float ActualFactor = Curve.GetScaledFactor(Distance);
					TestTrueExpr(FMath::IsNearlyEqual(ActualFactor, ExpectedFactor, FactorComparisonTolerance));
				});

				It(
					"Should return the expected factor, if the Distance is scaled between [Lower, Upper] * Factor, Factor -> [0.0, 1.0]",
					[this]
					{
						const auto& Lower = Curve.KeyPoints[1];
						const auto& Upper = Curve.KeyPoints[2];
						for (float Factor = 0.0; Factor <= 1.0; Factor += 0.1)
						{
							const float Distance = FMath::Lerp(Lower.DistanceInUnits, Upper.DistanceInUnits, Factor);
							const float ExpectedFactor = FMath::Lerp(Lower.DamageScaling, Upper.DamageScaling, Factor);
							const float ActualFactor = Curve.GetScaledFactor(Distance);
							TestTrueExpr(FMath::IsNearlyEqual(ActualFactor, ExpectedFactor, FactorComparisonTolerance));
						}
					});
			});
		});
	});
}
