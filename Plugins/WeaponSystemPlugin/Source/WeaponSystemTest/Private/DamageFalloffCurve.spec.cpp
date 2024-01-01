#include "DamageFalloffCurve.h"
#include "Algo/Compare.h"
#include "Misc/AutomationTest.h"

BEGIN_DEFINE_SPEC(FDamageFalloffCurve_Spec, "WeaponSystemPlugin.Runtime.DamageFalloffCurve",
                  EAutomationTestFlags::ApplicationContextMask
                  | EAutomationTestFlags::HighPriority | EAutomationTestFlags::ProductFilter)

TObjectPtr<UDamageFalloffCurve> Curve;

END_DEFINE_SPEC(FDamageFalloffCurve_Spec)

void FDamageFalloffCurve_Spec::Define()
{
	constexpr float FactorComparisonTolerance = 0.01;

	Describe("A Damage Falloff Curve", [this]
	{
		BeforeEach([this]
		{
			Curve = NewObject<UDamageFalloffCurve>();
			Curve->AddToRoot();
		});

		Describe("If empty", [this]
		{
			It("Should be valid", [this]
			{
				TestTrueExpr(Curve->IsValid());
			});

			It("Should return a scaling factor", [this]
			{
				TestTrueExpr(!FMath::IsNaN(Curve->GetDamageMultiplier(0)));
			});

			It("Shoudlnt require to be sorted", [this]
			{
				TestTrueExpr(!Curve->IsSortingRequired());
			});

			It("Shouldnt raise errors when sorted", [this]
			{
				Curve->SortKeyPoints();
			});
		});

		Describe("If it has keypoints", [this]
		{
			It("Shouldnt require sorting, if keypoints are added via the class methods", [this]
			{
				TArray KeyPointsToInsert
				{
					FDamageFalloffKeypoint{.DamageMultiplier = 1, .Distance = 1},
					FDamageFalloffKeypoint{.DamageMultiplier = 2, .Distance = 2},
					FDamageFalloffKeypoint{.DamageMultiplier = 3, .Distance = 3},
					FDamageFalloffKeypoint{.DamageMultiplier = 4, .Distance = 4},
				};
				auto begin = KeyPointsToInsert.rbegin();
				auto end = KeyPointsToInsert.rend();
				while (begin != end)
				{
					Curve->AddKeyPoint(*begin);
					TestTrueExpr(!Curve->IsSortingRequired());
					++begin;
				}
			});

			It("Shouldnt require sorting, if keypoints are inserted manually in the correct order", [this]
			{
				TArray KeyPointsToInsert
				{
					FDamageFalloffKeypoint{.DamageMultiplier = 1, .Distance = 1},
					FDamageFalloffKeypoint{.DamageMultiplier = 2, .Distance = 2},
					FDamageFalloffKeypoint{.DamageMultiplier = 3, .Distance = 3},
					FDamageFalloffKeypoint{.DamageMultiplier = 4, .Distance = 4},
				};
				Curve->KeyPoints.Append(KeyPointsToInsert);
				TestTrueExpr(!Curve->IsSortingRequired());
			});

			It("Should require sorting, if keypoints are inserted manually but without caring about the order", [this]
			{
				TArray KeyPointsToInsert
				{
					FDamageFalloffKeypoint{.DamageMultiplier = 4, .Distance = 4},
					FDamageFalloffKeypoint{.DamageMultiplier = 2, .Distance = 2},
					FDamageFalloffKeypoint{.DamageMultiplier = 1, .Distance = 1},
					FDamageFalloffKeypoint{.DamageMultiplier = 3, .Distance = 3},
				};
				Curve->KeyPoints.Append(KeyPointsToInsert);
				TestTrueExpr(Curve->IsSortingRequired());
			});

			It("Should sort the element properly", [this]
			{
				TArray KeyPointsToInsert
				{
					FDamageFalloffKeypoint{.DamageMultiplier = 4, .Distance = 4},
					FDamageFalloffKeypoint{.DamageMultiplier = 2, .Distance = 2},
					FDamageFalloffKeypoint{.DamageMultiplier = 1, .Distance = 1},
					FDamageFalloffKeypoint{.DamageMultiplier = 3, .Distance = 3},
				};
				TArray ExpectedKeyPoints
				{
					FDamageFalloffKeypoint{.DamageMultiplier = 1, .Distance = 1},
					FDamageFalloffKeypoint{.DamageMultiplier = 2, .Distance = 2},
					FDamageFalloffKeypoint{.DamageMultiplier = 3, .Distance = 3},
					FDamageFalloffKeypoint{.DamageMultiplier = 4, .Distance = 4},
				};

				Curve->KeyPoints.Append(KeyPointsToInsert);
				Curve->SortKeyPoints();
				TestTrueExpr(!Curve->IsSortingRequired());
				using FKeyPointConst = const FDamageFalloffKeypoint&;
				bool AreExpectedKeyPoints = Algo::Compare(Curve->KeyPoints, ExpectedKeyPoints,
				                                          [](FKeyPointConst A, FKeyPointConst B)
				                                          {
					                                          return std::tie(A.DamageMultiplier, A.Distance) ==
						                                          std::tie(
							                                          B.DamageMultiplier, B.Distance);
				                                          });
				TestTrueExpr(AreExpectedKeyPoints);
			});

			Describe("And you want the scaling factor", [this]
			{
				BeforeEach([this]
				{
					Curve->KeyPoints.Append(
						{
							FDamageFalloffKeypoint{.DamageMultiplier = 1, .Distance = 1},
							FDamageFalloffKeypoint{.DamageMultiplier = 2, .Distance = 2},
							FDamageFalloffKeypoint{.DamageMultiplier = 3, .Distance = 3},
							FDamageFalloffKeypoint{.DamageMultiplier = 4, .Distance = 4}
						}
					);
				});

				It("Should return zero, if Distance < First.Distance", [this]
				{
					const double Distance = Curve->KeyPoints[0].Distance - 1;
					TestTrueExpr(Curve->GetDamageMultiplier(Distance) == 0.0);
				});

				It("Should return First.Factor, if Distance == First.Distance", [this]
				{
					const auto& First = Curve->KeyPoints[0];
					const double Factor = Curve->GetDamageMultiplier(First.Distance);
					TestTrueExpr(FMath::IsNearlyEqual(First.DamageMultiplier, Factor, FactorComparisonTolerance));
				});

				It("Should return Lower.Factor, if Distance == Lower.Distance", [this]
				{
					const auto& Lower = Curve->KeyPoints[2];
					const double Factor = Curve->GetDamageMultiplier(Lower.Distance);
					TestTrueExpr(FMath::IsNearlyEqual(Lower.DamageMultiplier, Factor, FactorComparisonTolerance));
				});

				It("Should return Last.Factor, if Distance == Last.Distance", [this]
				{
					const auto& Last = Curve->KeyPoints.Last();
					const double Factor = Curve->GetDamageMultiplier(Last.Distance);
					TestTrueExpr(FMath::IsNearlyEqual(Last.DamageMultiplier, Factor,FactorComparisonTolerance));
				});


				It("Should return Last.Factor, if Distance > Last.Distance", [this]
				{
					const auto& Last = Curve->KeyPoints.Last();
					const double Factor = Curve->GetDamageMultiplier(Last.Distance + 1);
					TestTrueExpr(FMath::IsNearlyEqual(Last.DamageMultiplier, Factor, FactorComparisonTolerance));
				});

				It("Should return the expected factor, if Distance is half the one between Lower and Upper", [this]
				{
					const auto& Lower = Curve->KeyPoints[2];
					const auto& Upper = Curve->KeyPoints[3];
					const float ExpectedFactor = (Lower.DamageMultiplier + Upper.DamageMultiplier) / 2;
					const float Distance = (Lower.Distance + Upper.Distance) / 2;
					const float ActualFactor = Curve->GetDamageMultiplier(Distance);
					TestTrueExpr(FMath::IsNearlyEqual(ActualFactor, ExpectedFactor, FactorComparisonTolerance));
				});

				It(
					"Should return the expected factor, if the Distance is scaled between [Lower, Upper] * Factor, Factor -> [0.0, 1.0]",
					[this]
					{
						const auto& Lower = Curve->KeyPoints[1];
						const auto& Upper = Curve->KeyPoints[2];
						for (float Factor = 0.0; Factor <= 1.0; Factor += 0.1)
						{
							const float Distance = FMath::Lerp(Lower.Distance, Upper.Distance, Factor);
							const float ExpectedFactor = FMath::Lerp(Lower.DamageMultiplier, Upper.DamageMultiplier,
							                                         Factor);
							const float ActualFactor = Curve->GetDamageMultiplier(Distance);
							TestTrueExpr(FMath::IsNearlyEqual(ActualFactor, ExpectedFactor, FactorComparisonTolerance));
						}
					});
			});
		});

		AfterEach([this]
		{
			Curve->RemoveFromRoot();
		});
	});
}
