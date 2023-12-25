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
	TObjectPtr<UBallisticWeaponComponent> PrevComponent;

	UBallisticWeaponComponent* AttachComponent(UBallisticWeaponComponent* NewComponent)
	{
		if (PrevComponent)
		{
			DelegateHandler->UnRegister(PrevComponent);
			PrevComponent->DestroyComponent();
		}
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
			It("Shouldnt be ready to fire, if there is no ammo", [this]
			{
				auto const* Component = AttachComponent(NewObject<UBallisticWeaponComponent>(Actor));
				TestTrueExpr(
					Component->CurrentMagazine == 0
					&& Component->GetStatus() == EBallisticWeaponStatus::WaitingReload);
			});

			It("Shouldnt be ready to fire, if there is not enough ammo in the magazine", [this]
			{
				auto* Component = NewObject<UBallisticWeaponComponent>(Actor);
				Component->CurrentMagazine = 1;
				Component->AmmoUsedEachShot = 2;
				AttachComponent(Component);
				TestTrueExpr(Component->GetStatus() == EBallisticWeaponStatus::WaitingReload);
			});

			It("Should be ready to fire, if there is enough ammo in the magazine", [this]
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

		Describe("When trying to fire once", [this]
		{
			It("Cant fire because there is no ammo", [this]
			{
				auto* Component = AttachComponent(NewObject<UBallisticWeaponComponent>(Actor));
				Component->FireOnce();
				World.Tick();
				TestTrueExpr(DelegateHandler->OnShotFiredCounter == 0);
			});

			It("Cant fire because there is not enough ammo in the magazine", [this]
			{
				auto* Component = NewObject<UBallisticWeaponComponent>(Actor);
				Component->CurrentMagazine = 1;
				Component->AmmoUsedEachShot = 2;
				AttachComponent(Component);
				Component->FireOnce();
				World.Tick();
				TestTrueExpr(DelegateHandler->OnShotFiredCounter == 0);
			});

			It("Cant fire if not enough time has been passed between the previous shot", [this]
			{
				auto* Component = NewObject<UBallisticWeaponComponent>(Actor);
				Component->FireRateRpm = 60;
				Component->CurrentMagazine = 2;
				Component->AmmoUsedEachShot = 1;
				Component->AmmoType.IsHitScan = true;
				AttachComponent(Component);
				Component->FireOnce();
				World.Tick(0.125);
				Component->FireOnce();
				World.Tick();
				TestTrueExpr(DelegateHandler->OnShotFiredCounter == 1);
			});
		});
	});

	AfterEach([this]
	{
		if (PrevComponent)
			DelegateHandler->UnRegister(PrevComponent);
		DelegateHandler->ResetCounters();
		Actor->Destroy();
		Actor = nullptr;
		PrevComponent = nullptr;
	});
}

#endif
