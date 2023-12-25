#ifdef ICARUS_BUILD_TESTS

#include "BallisticWeaponComponentDelegateHandler.h"
#include "BallisticWeaponComponent.h"
#include "Misc/AutomationTest.h"
#include "IcarusTestSubsystem.h"

BEGIN_DEFINE_SPEC(FBallisticWeaponComponent_Spec, "Icarus.WeaponSystem.BallisticWeaponComponent",
                  EAutomationTestFlags::EditorContext | EAutomationTestFlags::ServerContext
                  | EAutomationTestFlags::HighPriority | EAutomationTestFlags::ProductFilter)

	TObjectPtr<UIcarusTestSubsystem> Subsystem;
	FIcarusTestWorldHelper World;
	TObjectPtr<UBallisticWeaponComponentDelegateHandler> DelegateHandler;
	TObjectPtr<AActor> Actor;

	UBallisticWeaponComponent* PrevComponent{};

	UBallisticWeaponComponent* AttachComponent(UBallisticWeaponComponent* NewComponent)
	{
		if (PrevComponent)
			DelegateHandler->UnRegister(PrevComponent);
		Actor->FinishAddComponent(NewComponent, false, FTransform::Identity);
		PrevComponent = NewComponent;
		if (NewComponent)
			DelegateHandler->Register(PrevComponent);
		return NewComponent;
	}

END_DEFINE_SPEC(FBallisticWeaponComponent_Spec)

void FBallisticWeaponComponent_Spec::Define()
{
	Describe("The Ballistic Weapon Component", [this]
	{
		BeforeEach([this]
		{
			if (!Subsystem)
				Subsystem = GEngine->GetEngineSubsystem<UIcarusTestSubsystem>();
			World = Subsystem->GetSharedWorld();
			if (!DelegateHandler)
				DelegateHandler = NewObject<UBallisticWeaponComponentDelegateHandler>();

			Actor = World->SpawnActor<AActor>();
		});

		Describe("When constructed", [this]
		{
			It("Shouldn't be ready to fire, if there's no ammo", [this]
			{
				auto const* Component = AttachComponent(NewObject<UBallisticWeaponComponent>(Actor));
				TestTrueExpr(
					Component->CurrentMagazine == 0 && Component->GetStatus() == EBallisticWeaponStatus::
					WaitingReload);
			});

			It("Should be ready to fire, if there's enough ammo in the magazine", [this]
			{
				auto* Component = NewObject<UBallisticWeaponComponent>(Actor);
				Component->CurrentMagazine = 1;
				Component->AmmoUsedEachShot = 1;
				AttachComponent(Component);
				TestTrueExpr(Component->GetStatus() == EBallisticWeaponStatus::Ready);
			});

			It("Should be ready to fire, if it has infinite ammo", [this]
			{
				auto* Component = NewObject<UBallisticWeaponComponent>(Actor);
				Component->HasInfiniteAmmo = true;
				AttachComponent(Component);
				TestTrueExpr(Component->GetStatus() == EBallisticWeaponStatus::Ready);
			});
		});
	});

	AfterEach([this]
	{
		if (PrevComponent)
			DelegateHandler->UnRegister(PrevComponent);
		DelegateHandler->ResetCounters();
		Actor->Destroy();
	});
}

#endif
