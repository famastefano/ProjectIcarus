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
	});
}
