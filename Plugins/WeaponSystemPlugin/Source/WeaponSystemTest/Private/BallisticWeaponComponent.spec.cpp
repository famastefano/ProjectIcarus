﻿#include "Logging/StructuredLog.h"

#include "BallisticWeaponComponentDelegateHandler.h"
#include "BallisticWeaponComponent.h"
#include "LogWeaponSystemTest.h"
#include "TestWorldActor.h"
#include "TestWorldSubsystem.h"

#include "Misc/AutomationTest.h"

BEGIN_DEFINE_SPEC(FBallisticWeaponComponent_Spec, "WeaponSystemPlugin.Runtime.BallisticWeaponComponent",
                  EAutomationTestFlags::ApplicationContextMask
                  | EAutomationTestFlags::HighPriority | EAutomationTestFlags::ProductFilter)

	TObjectPtr<UTestWorldSubsystem> Subsystem;
	FTestWorldHelper World;
	TObjectPtr<UBallisticWeaponComponentDelegateHandler> DelegateHandler;
	TObjectPtr<ATestWorldActor> Actor;
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
			const UBallisticWeaponComponent* CDO =
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

	UBallisticWeaponComponent* CreateAndAttachComponent(const FComponentOptions& Options = FComponentOptions())
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
			UE_LOGFMT(LogWeaponSystemTest, Log, "Test {Name} started.", GetTestFullName());
			if (!Subsystem)
			{
				Subsystem = GEngine->GetEngineSubsystem<UTestWorldSubsystem>();
			}
			World = Subsystem->GetSharedWorld();
			if (!DelegateHandler)
			{
				DelegateHandler = NewObject<UBallisticWeaponComponentDelegateHandler>();
			}

			Actor = World->SpawnActor<ATestWorldActor>();
		});

		Describe("When constructed", [this]
		{
			It("Shouldnt be ready to fire, if there is no ammo", [this]
			{
				const auto* Component = CreateAndAttachComponent();
				TestTrueExpr(
					Component->CurrentMagazine == 0
					&& Component->GetStatus() == EBallisticWeaponStatus::WaitingReload);
			});

			It("Shouldnt be ready to fire, if there is not enough ammo in the magazine", [this]
			{
				FComponentOptions Opt;
				Opt.CurrentMagazine = 1;
				Opt.AmmoUsedEachShot = 2;
				const auto* Component = CreateAndAttachComponent(Opt);
				TestTrueExpr(Component->GetStatus() == EBallisticWeaponStatus::WaitingReload);
			});

			It("Should be ready to fire, if there is enough ammo in the magazine", [this]
			{
				FComponentOptions Opt;
				Opt.CurrentMagazine = 1;
				Opt.AmmoUsedEachShot = 1;
				const auto* Component = CreateAndAttachComponent(Opt);
				TestTrueExpr(Component->GetStatus() == EBallisticWeaponStatus::Ready);
			});

			It("Should be ready to fire, if it has infinite ammo", [this]
			{
				FComponentOptions Opt;
				Opt.HasInfiniteAmmo = true;
				const auto* Component = CreateAndAttachComponent(Opt);
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

			It("Should allow to fire again if enough time has passed", [this]
			{
				FComponentOptions Opt;
				Opt.FireRateRpm = 600;
				Opt.HasInfiniteAmmo = true;
				Opt.AmmoType.IsHitScan = true;
				auto* Component = CreateAndAttachComponent(Opt);
				const double WaitingTimeBetweenEachShot = Component->GetSecondsBetweenShots() + 0.1;
				Component->FireOnce();
				World.Tick(WaitingTimeBetweenEachShot);
				Component->FireOnce();
				World.Tick(WaitingTimeBetweenEachShot);
				Component->FireOnce();
				World.Tick();
				TestTrueExpr(DelegateHandler->OnShotFiredCounter == 3);
			});
		});

		Describe("When reloading", [this]
		{
			It("Should notify Start and Completion", [this]
			{
				FComponentOptions Opt;
				Opt.AmmoReserve = 1;
				Opt.MagazineSize = 1;
				Opt.CurrentMagazine = 0;
				Opt.SecondsToReload = 0;
				auto* Component = CreateAndAttachComponent(Opt);
				Component->StartReloading();
				World.Tick();
				TestTrueExpr(DelegateHandler->OnReloadStartedCounter == 1
					&& DelegateHandler->OnReloadCompletedCounter == 1);
			});

			It("Should do so after a single Tick, if the reloading time is zero seconds", [this]
			{
				FComponentOptions Opt;
				Opt.AmmoReserve = 10;
				Opt.MagazineSize = 1;
				Opt.CurrentMagazine = 0;
				Opt.SecondsToReload = 0;
				auto* Component = CreateAndAttachComponent(Opt);
				Component->StartReloading();
				World.Tick(0.001);
				TestTrueExpr(Component->CurrentMagazine == Component->MagazineSize);
			});

			It("Should do so after the specified amount of time", [this]
			{
				FComponentOptions Opt;
				Opt.AmmoReserve = 10;
				Opt.MagazineSize = 1;
				Opt.CurrentMagazine = 0;
				Opt.SecondsToReload = 2;
				auto* Component = CreateAndAttachComponent(Opt);
				Component->StartReloading();
				World.Tick(0.5);
				World.Tick(0.5);
				World.Tick(0.5);
				World.Tick(0.5);
				World.Tick(0.5);
				TestTrueExpr(DelegateHandler->OnReloadStartedCounter == 1
					&& DelegateHandler->OnReloadCompletedCounter == 1);
			});

			It("Should allow to cancel the reload immediately", [this]
			{
				FComponentOptions Opt;
				Opt.AmmoReserve = 10;
				Opt.MagazineSize = 1;
				Opt.CurrentMagazine = 0;
				Opt.SecondsToReload = 0;
				auto* Component = CreateAndAttachComponent(Opt);
				Component->StartReloading();
				Component->CancelReloading();
				World.Tick(0.001);
				TestTrueExpr(Component->CurrentMagazine == 0
					&& DelegateHandler->OnReloadStartedCounter == 1
					&& DelegateHandler->OnReloadCanceledCounter == 1
					&& DelegateHandler->OnReloadCompletedCounter == 0);
			});

			It("Should allow to cancel the reload if it hasnt completed it", [this]
			{
				FComponentOptions Opt;
				Opt.AmmoReserve = 10;
				Opt.MagazineSize = 1;
				Opt.CurrentMagazine = 0;
				Opt.SecondsToReload = 1;
				auto* Component = CreateAndAttachComponent(Opt);
				Component->StartReloading();
				World.Tick(0.5);
				Component->CancelReloading();
				World.Tick(1);
				TestTrueExpr(Component->CurrentMagazine == 0
					&& DelegateHandler->OnReloadStartedCounter == 1
					&& DelegateHandler->OnReloadCanceledCounter == 1
					&& DelegateHandler->OnReloadCompletedCounter == 0);
			});

			It("Should fail if there isnt enough ammo in the reserve", [this]
			{
				FComponentOptions Opt;
				Opt.AmmoReserve = 0;
				Opt.MagazineSize = 1;
				Opt.CurrentMagazine = 0;
				Opt.SecondsToReload = 0;
				auto* Component = CreateAndAttachComponent(Opt);
				Component->StartReloading();
				World.Tick();
				TestTrueExpr(Component->CurrentMagazine == 0
					&& DelegateHandler->OnReloadStartedCounter == 1
					&& DelegateHandler->OnReloadFailedCounter == 1
					&& DelegateHandler->OnReloadCompletedCounter == 0);
			});

			It("Should decrease the ammo reserve with just enough ammo to fill the magazine", [this]
			{
				FComponentOptions Opt;
				Opt.AmmoReserve = 10;
				Opt.MagazineSize = 5;
				Opt.CurrentMagazine = 3;
				Opt.SecondsToReload = 0;
				auto* Component = CreateAndAttachComponent(Opt);
				Component->StartReloading();
				World.Tick();
				TestTrueExpr(Component->CurrentMagazine == 5 && Component->AmmoReserve == 8);
			});

			It("Should decrease the ammo reserve to fill the entire magazine, if asked", [this]
			{
				FComponentOptions Opt;
				Opt.AmmoReserve = 10;
				Opt.MagazineSize = 5;
				Opt.CurrentMagazine = 3;
				Opt.SecondsToReload = 0;
				Opt.ReloadingDiscardsEntireMagazine = true;
				auto* Component = CreateAndAttachComponent(Opt);
				Component->StartReloading();
				World.Tick();
				TestTrueExpr(Component->CurrentMagazine == 5 && Component->AmmoReserve == 5);
			});

			It(
				"Should fill the magazine with all the ammo reserve, if the reserve has less ammo than the amount required to fill the mag",
				[this]
				{
					FComponentOptions Opt;
					Opt.AmmoReserve = 1;
					Opt.MagazineSize = 5;
					Opt.CurrentMagazine = 3;
					Opt.SecondsToReload = 0;
					auto* Component = CreateAndAttachComponent(Opt);
					Component->StartReloading();
					World.Tick();
					TestTrueExpr(Component->CurrentMagazine == 4 && Component->AmmoReserve == 0);
				});

			It("Should refill the magazine if ammo reserves are infinite", [this]
			{
				FComponentOptions Opt;
				Opt.AmmoReserve = 0;
				Opt.MagazineSize = 10;
				Opt.CurrentMagazine = 5;
				Opt.SecondsToReload = 0;
				Opt.HasInfiniteAmmoReserve = true;
				auto* Component = CreateAndAttachComponent(Opt);
				Component->StartReloading();
				World.Tick();
				TestTrueExpr(Component->CurrentMagazine == 10);
			});

			It("Should refill the magazine if ammo reserves are infinite (while discarding the entire magazine)", [this]
			{
				FComponentOptions Opt;
				Opt.AmmoReserve = 0;
				Opt.MagazineSize = 10;
				Opt.CurrentMagazine = 5;
				Opt.SecondsToReload = 0;
				Opt.HasInfiniteAmmoReserve = true;
				Opt.ReloadingDiscardsEntireMagazine = true;
				auto* Component = CreateAndAttachComponent(Opt);
				Component->StartReloading();
				World.Tick();
				TestTrueExpr(Component->CurrentMagazine == 10);
			});
		});

		Describe("When firing continuously", [this]
		{
			Describe("In burst fire mode", [this]
			{
				It("Should shoot all the rounds, if it has enough ammo", [this]
				{
					FComponentOptions Opt;
					Opt.AmmoType.IsHitScan = true;
					Opt.IsBurstFire = true;
					Opt.ShotsFiredDuringBurstFire = 3;
					Opt.CurrentMagazine = 5;
					Opt.FireRateRpm = 600;
					auto* Component = CreateAndAttachComponent(Opt);
					const double DeltaTimeRequiredToShoot = Component->GetSecondsBetweenShots() + 0.1;
					Component->StartFiring();
					World.Tick(DeltaTimeRequiredToShoot);
					World.Tick(DeltaTimeRequiredToShoot);
					TestTrueExpr(Component->CurrentMagazine == 2 && DelegateHandler->OnShotFiredCounter == 3);
				});

				It("Should ask to reload if the magazine is depleted (shoots all burst fire rounds)", [this]
				{
					FComponentOptions Opt;
					Opt.AmmoType.IsHitScan = true;
					Opt.IsBurstFire = true;
					Opt.ShotsFiredDuringBurstFire = 2;
					Opt.CurrentMagazine = 2;
					Opt.FireRateRpm = 600;
					auto* Component = CreateAndAttachComponent(Opt);
					const double DeltaTimeRequiredToShoot = Component->GetSecondsBetweenShots() + 0.1;
					Component->StartFiring();
					World.Tick(DeltaTimeRequiredToShoot);
					TestTrueExpr(Component->CurrentMagazine == 0
						&& DelegateHandler->OnShotFiredCounter == 2
						&& DelegateHandler->OnReloadRequestedCounter == 1
					);
				});

				It("Should ask to reload if the magazine is depleted (interrupts burst firing)", [this]
				{
					FComponentOptions Opt;
					Opt.AmmoType.IsHitScan = true;
					Opt.IsBurstFire = true;
					Opt.ShotsFiredDuringBurstFire = 10;
					Opt.CurrentMagazine = 8;
					Opt.FireRateRpm = 600;
					auto* Component = CreateAndAttachComponent(Opt);
					const double DeltaTimeRequiredToShoot = Component->GetSecondsBetweenShots() + 0.1;
					Component->StartFiring();
					World.TickUntil(DeltaTimeRequiredToShoot, [RoundsToShoot = Opt.CurrentMagazine]() mutable
					{
						return --RoundsToShoot == 0;
					});
					TestTrueExpr(Component->CurrentMagazine == 0
						&& DelegateHandler->OnShotFiredCounter == 8
						&& DelegateHandler->OnReloadRequestedCounter == 1
					);
				});

				It("Should interrupt burst fire if requested", [this]
				{
					FComponentOptions Opt;
					Opt.AmmoType.IsHitScan = true;
					Opt.IsBurstFire = true;
					Opt.ShotsFiredDuringBurstFire = 10;
					Opt.CurrentMagazine = 8;
					Opt.FireRateRpm = 600;
					auto* Component = CreateAndAttachComponent(Opt);
					const double DeltaTimeRequiredToShoot = Component->GetSecondsBetweenShots() + 0.1;
					Component->StartFiring();
					World.Tick(DeltaTimeRequiredToShoot);
					World.Tick(DeltaTimeRequiredToShoot);
					Component->StopFiring();
					World.Tick(DeltaTimeRequiredToShoot);
					TestTrueExpr(Component->CurrentMagazine == 5 && DelegateHandler->OnShotFiredCounter == 3);
				});

				It("Should start a new burst firing sequence each time we start firing", [this]
				{
					FComponentOptions Opt;
					Opt.AmmoType.IsHitScan = true;
					Opt.IsBurstFire = true;
					Opt.ShotsFiredDuringBurstFire = 2;
					Opt.CurrentMagazine = 8;
					Opt.FireRateRpm = 600;
					auto* Component = CreateAndAttachComponent(Opt);
					const double DeltaTimeRequiredToShoot = Component->GetSecondsBetweenShots() + 0.1;
					for (int i = 0; i < Opt.CurrentMagazine / Opt.ShotsFiredDuringBurstFire; ++i)
					{
						Component->StartFiring();
						World.Tick(DeltaTimeRequiredToShoot);
						World.Tick(DeltaTimeRequiredToShoot);
					}
					TestTrueExpr(Component->CurrentMagazine == 0
						&& DelegateHandler->OnShotFiredCounter == 8
						&& DelegateHandler->OnFiringStartedCounter == 4
						&& DelegateHandler->OnFiringStoppedCounter == 4
						&& DelegateHandler->OnReloadRequestedCounter == 1);
				});
			});

			Describe("In automatic mode", [this]
			{
				It("Wont start if the magazine is empty", [this]
				{
					FComponentOptions Opt;
					Opt.CurrentMagazine = 0;
					Opt.AmmoUsedEachShot = 1;
					auto* Component = CreateAndAttachComponent(Opt);
					Component->StartFiring();
					World.Tick();
					TestTrueExpr(DelegateHandler->OnFiringStartedCounter == 0
						&& DelegateHandler->OnShotFiredCounter == 0);
				});

				It("Wont start if the magazine has less rounds than the required ones to shot", [this]
				{
					FComponentOptions Opt;
					Opt.CurrentMagazine = 5;
					Opt.AmmoUsedEachShot = Opt.CurrentMagazine + 1;
					auto* Component = CreateAndAttachComponent(Opt);
					Component->StartFiring();
					World.Tick();
					TestTrueExpr(DelegateHandler->OnFiringStartedCounter == 0
						&& DelegateHandler->OnShotFiredCounter == 0);
				});

				It("Will shoot until the magazine is empty", [this]
				{
					FComponentOptions Opt;
					Opt.CurrentMagazine = 5;
					Opt.AmmoUsedEachShot = 1;
					Opt.AmmoType.IsHitScan = true;
					Opt.FireRateRpm = 600;
					auto* Component = CreateAndAttachComponent(Opt);
					double TimeToShoot = Component->GetSecondsBetweenShots() + 0.1;
					Component->StartFiring();
					World.Tick(TimeToShoot);
					World.Tick(TimeToShoot);
					World.Tick(TimeToShoot);
					World.Tick(TimeToShoot);
					TestTrueExpr(Component->CurrentMagazine == 0
						&& DelegateHandler->OnShotFiredCounter == 5
						&& DelegateHandler->OnFiringStartedCounter == 1
						&& DelegateHandler->OnFiringStoppedCounter == 1
						&& DelegateHandler->OnReloadRequestedCounter == 1);
				});
			});
		});

		AfterEach([this]
		{
			if (PrevComponent)
			{
				DelegateHandler->UnRegister(PrevComponent);
			}
			DelegateHandler->ResetCounters();
			Actor->Destroy();
			Actor = nullptr;
			PrevComponent = nullptr;
			UE_LOGFMT(LogWeaponSystemTest, Log, "Test ended.");
		});
	});
}
