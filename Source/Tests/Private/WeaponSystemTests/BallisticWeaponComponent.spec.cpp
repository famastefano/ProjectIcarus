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

	struct FComponentOptions
	{
		decltype(UBallisticWeaponComponent::HasInfiniteAmmo) HasInfiniteAmmo;
		decltype(UBallisticWeaponComponent::HasInfiniteAmmoReserve) HasInfiniteAmmoReserve;
		decltype(UBallisticWeaponComponent::ReloadingDiscardsEntireMagazine) ReloadingDiscardsEntireMagazine;
		decltype(UBallisticWeaponComponent::MagazineSize) MagazineSize;
		decltype(UBallisticWeaponComponent::AmmoUsedEachShot) AmmoUsedEachShot;
		decltype(UBallisticWeaponComponent::CurrentMagazine) CurrentMagazine;
		decltype(UBallisticWeaponComponent::AmmoReserve) AmmoReserve;
		decltype(UBallisticWeaponComponent::FireRateRpm) FireRateRpm;
		decltype(UBallisticWeaponComponent::IsBurstFire) IsBurstFire;
		decltype(UBallisticWeaponComponent::ShotsFiredDuringBurstFire) ShotsFiredDuringBurstFire;
		decltype(UBallisticWeaponComponent::SecondsToReload) SecondsToReload;
		decltype(UBallisticWeaponComponent::AmmoType) AmmoType;

		FComponentOptions()
		{
			UBallisticWeaponComponent const* CDO =
				UBallisticWeaponComponent::StaticClass()
				->GetDefaultObject<UBallisticWeaponComponent>();
			HasInfiniteAmmo = CDO->HasInfiniteAmmo;
			HasInfiniteAmmoReserve = CDO->HasInfiniteAmmoReserve;
			ReloadingDiscardsEntireMagazine = CDO->ReloadingDiscardsEntireMagazine;
			MagazineSize = CDO->MagazineSize;
			AmmoUsedEachShot = CDO->AmmoUsedEachShot;
			CurrentMagazine = CDO->CurrentMagazine;
			AmmoReserve = CDO->AmmoReserve;
			FireRateRpm = CDO->FireRateRpm;
			IsBurstFire = CDO->IsBurstFire;
			ShotsFiredDuringBurstFire = CDO->ShotsFiredDuringBurstFire;
			SecondsToReload = CDO->SecondsToReload;
			AmmoType = CDO->AmmoType;
		}
	};

	UBallisticWeaponComponent* CreateAndAttachComponent(FComponentOptions const& Options = FComponentOptions())
	{
		check(IsInGameThread());
		if (PrevComponent)
		{
			DelegateHandler->UnRegister(PrevComponent);
			PrevComponent->DestroyComponent();
		}

		PrevComponent = NewObject<UBallisticWeaponComponent>(Actor);
		check(PrevComponent);
		PrevComponent->HasInfiniteAmmo = Options.HasInfiniteAmmo;
		PrevComponent->HasInfiniteAmmoReserve = Options.HasInfiniteAmmoReserve;
		PrevComponent->ReloadingDiscardsEntireMagazine = Options.ReloadingDiscardsEntireMagazine;
		PrevComponent->MagazineSize = Options.MagazineSize;
		PrevComponent->AmmoUsedEachShot = Options.AmmoUsedEachShot;
		PrevComponent->CurrentMagazine = Options.CurrentMagazine;
		PrevComponent->AmmoReserve = Options.AmmoReserve;
		PrevComponent->FireRateRpm = Options.FireRateRpm;
		PrevComponent->IsBurstFire = Options.IsBurstFire;
		PrevComponent->ShotsFiredDuringBurstFire = Options.ShotsFiredDuringBurstFire;
		PrevComponent->SecondsToReload = Options.SecondsToReload;
		PrevComponent->AmmoType = Options.AmmoType;

		Actor->FinishAddComponent(PrevComponent, false, FTransform::Identity);
		DelegateHandler->Register(PrevComponent);
		return PrevComponent;
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
				auto const* Component = CreateAndAttachComponent();
				TestTrueExpr(
					Component->CurrentMagazine == 0
					&& Component->GetStatus() == EBallisticWeaponStatus::WaitingReload);
			});

			It("Shouldnt be ready to fire, if there is not enough ammo in the magazine", [this]
			{
				FComponentOptions Opt;
				Opt.CurrentMagazine = 1;
				Opt.AmmoUsedEachShot = 2;
				auto const* Component = CreateAndAttachComponent(Opt);
				TestTrueExpr(Component->GetStatus() == EBallisticWeaponStatus::WaitingReload);
			});

			It("Should be ready to fire, if there is enough ammo in the magazine", [this]
			{
				FComponentOptions Opt;
				Opt.CurrentMagazine = 1;
				Opt.AmmoUsedEachShot = 1;
				auto const* Component = CreateAndAttachComponent(Opt);
				TestTrueExpr(Component->GetStatus() == EBallisticWeaponStatus::Ready);
			});

			It("Should be ready to fire, if it has infinite ammo", [this]
			{
				FComponentOptions Opt;
				Opt.HasInfiniteAmmo = true;
				auto const* Component = CreateAndAttachComponent(Opt);
				TestTrueExpr(Component->GetStatus() == EBallisticWeaponStatus::Ready);
			});
		});

		Describe("When trying to fire once", [this]
		{
			It("Cant fire because there is no ammo", [this]
			{
				auto* Component = CreateAndAttachComponent();
				Component->FireOnce();
				World.Tick();
				TestTrueExpr(DelegateHandler->OnShotFiredCounter == 0);
			});

			It("Cant fire because there is not enough ammo in the magazine", [this]
			{
				FComponentOptions Opt;
				Opt.CurrentMagazine = 1;
				Opt.AmmoUsedEachShot = 2;
				auto* Component = CreateAndAttachComponent(Opt);
				Component->FireOnce();
				World.Tick();
				TestTrueExpr(DelegateHandler->OnShotFiredCounter == 0);
			});

			It("Cant fire if not enough time has been passed between the previous shot", [this]
			{
				FComponentOptions Opt;
				Opt.FireRateRpm = 60;
				Opt.CurrentMagazine = 2;
				Opt.AmmoUsedEachShot = 1;
				Opt.AmmoType.IsHitScan = true;
				auto* Component = CreateAndAttachComponent(Opt);
				Component->FireOnce();
				World.Tick(0.125);
				Component->FireOnce();
				World.Tick();
				TestTrueExpr(DelegateHandler->OnShotFiredCounter == 1);
			});

			It("Should fire, if ammo are infinite", [this]
			{
				FComponentOptions Opt;
				Opt.HasInfiniteAmmo = true;
				Opt.AmmoType.IsHitScan = true;
				auto* Component = CreateAndAttachComponent(Opt);
				Component->FireOnce();
				World.Tick();
				TestTrueExpr(DelegateHandler->OnShotFiredCounter == 1);
			});

			It("Shouldnt modify the magazine if ammo are infinite", [this]
			{
				FComponentOptions Opt;
				Opt.HasInfiniteAmmo = true;
				Opt.CurrentMagazine = 1;
				Opt.AmmoUsedEachShot = 2;
				Opt.AmmoType.IsHitScan = true;
				auto* Component = CreateAndAttachComponent(Opt);
				Component->FireOnce();
				World.Tick();
				TestTrueExpr(DelegateHandler->OnShotFiredCounter == 1);
				TestTrueExpr(Component->CurrentMagazine == 1);
			});

			It("Shouldnt modify the magazine, if no ammo are consumed each shot", [this]
			{
				FComponentOptions Opt;
				Opt.CurrentMagazine = 10;
				Opt.AmmoUsedEachShot = 0;
				Opt.AmmoType.IsHitScan = true;
				auto* Component = CreateAndAttachComponent(Opt);
				Component->FireOnce();
				World.Tick();
				TestTrueExpr(DelegateHandler->OnShotFiredCounter == 1);
				TestTrueExpr(Component->CurrentMagazine == 10);
			});

			It("Should update the magazine, if it consumes ammo each shot", [this]
			{
				FComponentOptions Opt;
				Opt.CurrentMagazine = 10;
				Opt.AmmoUsedEachShot = 8;
				Opt.AmmoType.IsHitScan = true;
				auto* Component = CreateAndAttachComponent(Opt);
				Component->FireOnce();
				World.Tick();
				TestTrueExpr(DelegateHandler->OnShotFiredCounter == 1);
				TestTrueExpr(Component->CurrentMagazine == 2);
			});

			It("Should request to reload, if the magazine is depleted", [this]
			{
				FComponentOptions Opt;
				Opt.CurrentMagazine = 2;
				Opt.AmmoUsedEachShot = 2;
				Opt.AmmoType.IsHitScan = true;
				auto* Component = CreateAndAttachComponent(Opt);
				Component->FireOnce();
				World.Tick();
				TestTrueExpr(DelegateHandler->OnShotFiredCounter == 1);
				TestTrueExpr(DelegateHandler->OnReloadRequestedCounter == 1);
				TestTrueExpr(Component->CurrentMagazine == 0);
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
