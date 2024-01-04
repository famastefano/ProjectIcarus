#include "ActorPoolSubsystem.h"

#include "Logging/StructuredLog.h"

#include "LogObjectPoolingSystemTest.h"

#include "TestWorldSubsystem.h"

#include "Misc/AutomationTest.h"

BEGIN_DEFINE_SPEC(FActorPoolSubsystem_Spec, "ObjectPoolingSystem.Runtime.ActorPooling",
                  EAutomationTestFlags::ApplicationContextMask
                  | EAutomationTestFlags::MediumPriority
                  | EAutomationTestFlags::ProductFilter)

	TObjectPtr<UTestWorldSubsystem> TestSubsystem;
	TObjectPtr<AActor> WorldContextObject;
	FTestWorldHelper World;

END_DEFINE_SPEC(FActorPoolSubsystem_Spec)

void FActorPoolSubsystem_Spec::Define()
{
	Describe("The Actor Pool", [this]
	{
		BeforeEach([this]
		{
			UE_LOGFMT(LogObjectPoolingSystemTest, Log, "Test {Name} started.", GetTestFullName());
			if (!TestSubsystem)
			{
				TestSubsystem = GEngine->GetEngineSubsystem<UTestWorldSubsystem>();
			}
			World = TestSubsystem->GetSharedWorld();
			WorldContextObject = World->SpawnActor<AActor>();
		});

		It("Should be empty, if it has never been used", [this]
		{
			const auto& Stats = UActorPoolSubsystem::GetAllPoolStats(WorldContextObject);
			TestTrueExpr(Stats.IsEmpty());
		});

		AfterEach([this]
		{
			UActorPoolSubsystem::EmptyPools(WorldContextObject);
			WorldContextObject->Destroy();
			UE_LOGFMT(LogObjectPoolingSystemTest, Log, "Test ended.");
		});
	});
}
