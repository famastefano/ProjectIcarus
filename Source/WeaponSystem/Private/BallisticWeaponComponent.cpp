// Stefano Famà (famastefano@gmail.com)


#include "BallisticWeaponComponent.h"

#include "AmmoType.h"

#include "Components/ArrowComponent.h"
#include "DrawDebugHelpers.h"
#include "LogWeaponSystem.h"

#include "Engine/DamageEvents.h"

#include "Logging/StructuredLog.h"

UBallisticWeaponComponent::UBallisticWeaponComponent()
{
	bAllowReregistration = true;
	bAutoRegister = true;

	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickInterval = 0;

#if WITH_EDITORONLY_DATA
	BulletExitDirection = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("Bullet_Exit_Direction"));
	if (ensureMsgf(BulletExitDirection != nullptr,
	               TEXT("Couldn't create ArrowComponent to visualize the WeaponComponent")))
	{
		BulletExitDirection->ArrowColor = FColor::Red;
		BulletExitDirection->bTreatAsASprite = true;
		BulletExitDirection->SpriteInfo.Category = "Weapon Components";
		BulletExitDirection->SpriteInfo.DisplayName = FText::FromStringView(TEXT("Bullet Exit Direction"));
		BulletExitDirection->SetupAttachment(this);
		BulletExitDirection->bIsScreenSizeScaled = true;
		BulletExitDirection->SetSimulatePhysics(false);
	}
#endif
}

void UBallisticWeaponComponent::BeginPlay()
{
	UE_LOGFMT(LogWeaponSystem, Verbose, "UBallisticWeaponComponent `{Name}` BeginPlay.", GetFName());
	SecondsBetweenEachShot = GetSecondsBetweenShots();
	LastFireTimestamp = -SecondsBetweenEachShot;
	Status = HasEnoughAmmoToFire() ? EBallisticWeaponStatus::Ready : EBallisticWeaponStatus::WaitingReload;
	StatusNotificationQueue.HasStatusChanged = 0;
	Super::BeginPlay();
}

EBallisticWeaponStatus UBallisticWeaponComponent::GetStatus() const
{
	return Status;
}

void UBallisticWeaponComponent::SetFireRate(int NewFireRateInRpm)
{
#if !UE_BUILD_SHIPPING
	if (NewFireRateInRpm <= 0)
	{
		UE_LOGFMT(LogWeaponSystem, Error, "Invalid new fire rate: {NewFireRate} RPM.", NewFireRateInRpm);
	}
#endif

	FireRateRpm = NewFireRateInRpm;
	SecondsBetweenEachShot = GetSecondsBetweenShots();
}

void UBallisticWeaponComponent::FireOnce()
{
	if (Status == EBallisticWeaponStatus::Ready && HasEnoughAmmoToFire() && HasEnoughTimePassedFromLastShot())
	{
		if (IsBurstFire)
		{
			Status = EBallisticWeaponStatus::Firing;
			StatusNotificationQueue.Queue.NotifyOnFiringStarted |= 1;
		}
		Fire();
		UpdateMagazineAfterFiring();
		NotifyStatusUpdate();
	}
}

void UBallisticWeaponComponent::StartFiring()
{
	if (Status == EBallisticWeaponStatus::Ready && HasEnoughAmmoToFire() && HasEnoughTimePassedFromLastShot())
	{
		Status = EBallisticWeaponStatus::Firing;
		StatusNotificationQueue.Queue.NotifyOnFiringStarted |= 1;
		if (IsBurstFire)
		{
			CurrentBurstFiringCount = ShotsFiredDuringBurstFire;
		}
		Fire();
		UpdateMagazineAfterFiring();
		NotifyStatusUpdate();
	}
}

void UBallisticWeaponComponent::StopFiring()
{
	if (Status == EBallisticWeaponStatus::Firing)
	{
		Status = HasEnoughAmmoToFire() ? EBallisticWeaponStatus::Ready : EBallisticWeaponStatus::WaitingReload;
		StatusNotificationQueue.Queue.NotifyOnFiringStopped |= 1;
		StatusNotificationQueue.Queue.NotifyOnReloadRequested |= Status == EBallisticWeaponStatus::WaitingReload;
		NotifyStatusUpdate();
	}
}

void UBallisticWeaponComponent::StartReloading()
{
	Status = EBallisticWeaponStatus::Reloading;
	StatusNotificationQueue.Queue.NotifyOnReloadStarted |= 1;
	ReloadTimestamp = GetWorld()->TimeSeconds + SecondsToReload;
}

void UBallisticWeaponComponent::CancelReloading()
{
	if (Status == EBallisticWeaponStatus::Reloading)
	{
		Status = HasEnoughAmmoToFire() ? EBallisticWeaponStatus::Ready : EBallisticWeaponStatus::WaitingReload;
		StatusNotificationQueue.Queue.NotifyOnReloadCanceled |= 1;
		StatusNotificationQueue.Queue.NotifyOnReloadRequested |= Status == EBallisticWeaponStatus::WaitingReload;
	}
}

double UBallisticWeaponComponent::GetSecondsBetweenShots() const
{
	check(FireRateRpm > 0);
	return 1.0 / (FireRateRpm / 60.0);
}

bool UBallisticWeaponComponent::HasEnoughAmmoToFire() const
{
	return HasInfiniteAmmo || CurrentMagazine >= AmmoUsedEachShot;
}

bool UBallisticWeaponComponent::HasEnoughTimePassedFromLastShot() const
{
	return GetWorld()->TimeSeconds >= LastFireTimestamp + SecondsBetweenEachShot;
}

void UBallisticWeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                              FActorComponentTickFunction* ThisTickFunction)
{
	UE_LOGFMT(LogWeaponSystem, VeryVerbose, "UBallisticWeaponComponent::TickComponent: {DeltaTime} s.", DeltaTime);

	if (Status == EBallisticWeaponStatus::Firing)
	{
		if (HasEnoughAmmoToFire())
		{
			if (HasEnoughTimePassedFromLastShot())
			{
				if (!IsBurstFire || CurrentBurstFiringCount > 0)
				{
					Fire();
					UpdateMagazineAfterFiring();
				}
			}
		}
		else
		{
			Status = EBallisticWeaponStatus::WaitingReload;
			StatusNotificationQueue.Queue.NotifyOnReloadRequested |= 1;
			StatusNotificationQueue.Queue.NotifyOnFiringStopped |= Status == EBallisticWeaponStatus::Firing;
		}
	}
	else if (Status == EBallisticWeaponStatus::Reloading && GetWorld()->TimeSeconds >= ReloadTimestamp)
	{
		ReloadMagazine();
		Status = HasEnoughAmmoToFire() ? EBallisticWeaponStatus::Ready : EBallisticWeaponStatus::WaitingReload;
		StatusNotificationQueue.Queue.NotifyOnReloadRequested |= Status == EBallisticWeaponStatus::WaitingReload;
	}

	NotifyStatusUpdate();
}

void UBallisticWeaponComponent::Fire()
{
	UWorld* World = GetWorld();
	checkf(World, TEXT("Tried to fire a weapon without a World available."));
	const FVector MuzzleLocation = GetComponentLocation();
	const FVector MuzzleDirection = GetForwardVector();

	LastFireTimestamp = World->TimeSeconds;

	if (AmmoType.IsHitScan)
	{
		// TODO: Support DamageFalloffCurve after the proper editor has been created
		constexpr float MaximumDistance = 100'000; // 1000 m

#if !UE_BUILD_SHIPPING
		DrawDebugLine(World, MuzzleLocation, MuzzleLocation + MuzzleDirection * MaximumDistance,
		              FColor::Red, false, 1.f, 0, 0.5f);
#endif

		// Async line trace is too imprecise, so we've chosen to use the sync one that yields very close results to expected RPMs
		FHitResult HitResult;
		if (World->LineTraceSingleByChannel(
			HitResult,
			MuzzleLocation,
			MuzzleLocation + MuzzleDirection * MaximumDistance,
			AmmoType.CollisionChannel))
		{
			UE_LOGFMT(LogWeaponSystem, Verbose, "Hit {Actor}, {Distance} cm far",
			          HitResult.GetActor()->GetName(),
			          HitResult.Distance);

			if (AActor* ActorHit = HitResult.GetActor(); ActorHit && ActorHit->CanBeDamaged())
			{
				// TODO: Calculate damage falloff based on distance
				AActor* Owner = GetOwner();
				const FDamageEvent DamageEvent{AmmoType.DamageType};
				ActorHit->TakeDamage(AmmoType.DamageAmount, DamageEvent, Owner->GetInstigatorController(), Owner);
			}
		}
	}
	else
	{
#if !UE_BUILD_SHIPPING
		if (!AmmoType.ProjectileClass)
		{
			UE_LOGFMT(LogWeaponSystem, Error, "AmmoType can't fire a projectile if ProjectileClass isn't set!");
		}
#endif
		// TODO: Spawn Projectile Component
	}

	StatusNotificationQueue.Queue.NotifyOnShotFired |= 1;

	if (IsBurstFire)
	{
		if (--CurrentBurstFiringCount <= 0 && Status == EBallisticWeaponStatus::Firing)
		{
			Status = HasEnoughAmmoToFire() ? EBallisticWeaponStatus::Ready : EBallisticWeaponStatus::WaitingReload;
			StatusNotificationQueue.Queue.NotifyOnFiringStopped |= 1;
			StatusNotificationQueue.Queue.NotifyOnReloadRequested |= Status == EBallisticWeaponStatus::WaitingReload;
		}
	}
}

void UBallisticWeaponComponent::UpdateMagazineAfterFiring()
{
	if (LIKELY(!HasInfiniteAmmo))
	{
		CurrentMagazine -= AmmoUsedEachShot;
#if !UE_BUILD_SHIPPING
		if (CurrentMagazine < 0)
		{
			UE_LOGFMT(LogWeaponSystem, Error,
			          "Invalid magazine, this weapon shouldn't have fired at all. Magazine {Magazine}, Rounds per shot {RoundsPerShot}.",
			          CurrentMagazine, AmmoUsedEachShot);
		}
#endif
		if (CurrentMagazine <= 0 && Status != EBallisticWeaponStatus::WaitingReload)
		{
			StatusNotificationQueue.Queue.NotifyOnFiringStopped |= Status == EBallisticWeaponStatus::Firing;
			Status = EBallisticWeaponStatus::WaitingReload;
			StatusNotificationQueue.Queue.NotifyOnReloadRequested |= 1;
		}
	}
}

void UBallisticWeaponComponent::ReloadMagazine()
{
	if (ReloadingDiscardsEntireMagazine)
	{
		CurrentMagazine = 0;
	}

	checkf(MagazineSize >= CurrentMagazine, TEXT("Precondition MagazineSize >= CurrentMagazine (%d >= %d) is false."),
	       MagazineSize, CurrentMagazine);

	if (LIKELY(!HasInfiniteAmmoReserve))
	{
		const int16 MissingRounds = MagazineSize - CurrentMagazine;
		const int16 RoundsAvailable = AmmoReserve >= MissingRounds ? MissingRounds : AmmoReserve;
		if (RoundsAvailable > 0)
		{
			CurrentMagazine += RoundsAvailable;
			AmmoReserve -= RoundsAvailable;
			StatusNotificationQueue.Queue.NotifyOnReloadCompleted |= 1;
		}
		else
		{
			StatusNotificationQueue.Queue.NotifyOnReloadFailed |= 1;
		}
	}
	else
	{
		CurrentMagazine = MagazineSize;
		StatusNotificationQueue.Queue.NotifyOnReloadCompleted |= 1;
	}
}

void UBallisticWeaponComponent::NotifyStatusUpdate()
{
	if (StatusNotificationQueue.HasStatusChanged == 0)
	{
		return;
	}

	if (OnStatusChanged.IsBound())
	{
		OnStatusChanged.Broadcast(Status);
	}

	if (StatusNotificationQueue.Queue.NotifyOnReloadRequested && OnReloadRequested.IsBound())
	{
		OnReloadRequested.Broadcast();
	}
	if (StatusNotificationQueue.Queue.NotifyOnReloadStarted && OnReloadStarted.IsBound())
	{
		OnReloadStarted.Broadcast();
	}
	if (StatusNotificationQueue.Queue.NotifyOnReloadCanceled && OnReloadCanceled.IsBound())
	{
		OnReloadCanceled.Broadcast();
	}
	if (StatusNotificationQueue.Queue.NotifyOnReloadCompleted && OnReloadCompleted.IsBound())
	{
		OnReloadCompleted.Broadcast();
	}
	if (StatusNotificationQueue.Queue.NotifyOnReloadFailed && OnReloadFailed.IsBound())
	{
		OnReloadFailed.Broadcast();
	}
	if (StatusNotificationQueue.Queue.NotifyOnFiringStarted && OnFiringStarted.IsBound())
	{
		OnFiringStarted.Broadcast();
	}
	if (StatusNotificationQueue.Queue.NotifyOnFiringStopped && OnFiringStopped.IsBound())
	{
		OnFiringStopped.Broadcast();
	}
	if (StatusNotificationQueue.Queue.NotifyOnShotFired && OnShotFired.IsBound())
	{
		OnShotFired.Broadcast();
	}

	StatusNotificationQueue.HasStatusChanged = 0;
}
