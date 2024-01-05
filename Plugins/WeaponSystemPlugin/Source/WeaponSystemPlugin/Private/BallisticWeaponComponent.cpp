// Stefano Famà (famastefano@gmail.com)


#include "BallisticWeaponComponent.h"

#include "Components/ArrowComponent.h"
#include "DrawDebugHelpers.h"
#include "LogWeaponSystem.h"
#include "WeaponSystemTrace.h"

#include "Engine/DamageEvents.h"

#include "Logging/StructuredLog.h"

#include "ProfilingDebugging/MiscTrace.h"

#include "ActorPoolSubsystem.h"

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
	StatusNotificationQueue = {};
	if (FiringStrategy == EBallisticWeaponFiringStrategy::Automatic)
	{
		FiringStrategy = FireRateRpm <= 1200
			                 ? EBallisticWeaponFiringStrategy::Timestamp
			                 : EBallisticWeaponFiringStrategy::TimestampWithAccumulator;
	}
	SetFiringStrategy(FiringStrategy);
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

	TRACE_BOOKMARK(TEXT("BallisticWeapon.SetFireRate(%d): %s"), NewFireRateInRpm, *GetNameSafe(GetOwner()));
	FireRateRpm = NewFireRateInRpm;
	SecondsBetweenEachShot = GetSecondsBetweenShots();
}

void UBallisticWeaponComponent::FireOnce()
{
	if (Status == EBallisticWeaponStatus::Ready && HasEnoughAmmoToFire() && HasEnoughTimePassedFromLastShot())
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL_STR("Ballistic Weapon FireOnce", WeaponSystemChannel);
		if (IsBurstFire)
		{
			Status = EBallisticWeaponStatus::Firing;
			StatusNotificationQueue.NotifyOnFiringStarted |= 1;
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
		TRACE_BOOKMARK(TEXT("BallisticWeapon.StartFiring: %s"), *GetNameSafe(GetOwner()));
		Status = EBallisticWeaponStatus::Firing;
		StatusNotificationQueue.NotifyOnFiringStarted |= 1;
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
		TRACE_BOOKMARK(TEXT("BallisticWeapon.StopFiring: %s"), *GetNameSafe(GetOwner()));
		Status = HasEnoughAmmoToFire() ? EBallisticWeaponStatus::Ready : EBallisticWeaponStatus::WaitingReload;
		StatusNotificationQueue.NotifyOnFiringStopped |= 1;
		StatusNotificationQueue.NotifyOnReloadRequested |= Status == EBallisticWeaponStatus::WaitingReload;
		NotifyStatusUpdate();
	}
}

void UBallisticWeaponComponent::StartReloading()
{
	Status = EBallisticWeaponStatus::Reloading;
	StatusNotificationQueue.NotifyOnReloadStarted |= 1;
	ReloadTimestamp = GetWorld()->TimeSeconds + SecondsToReload;
}

void UBallisticWeaponComponent::CancelReloading()
{
	if (Status == EBallisticWeaponStatus::Reloading)
	{
		Status = HasEnoughAmmoToFire() ? EBallisticWeaponStatus::Ready : EBallisticWeaponStatus::WaitingReload;
		StatusNotificationQueue.NotifyOnReloadCanceled |= 1;
		StatusNotificationQueue.NotifyOnReloadRequested |= Status == EBallisticWeaponStatus::WaitingReload;
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
	switch (FiringStrategy)
	{
	default:
	case EBallisticWeaponFiringStrategy::Timestamp:
	case EBallisticWeaponFiringStrategy::TimestampWithAccumulator:
	case EBallisticWeaponFiringStrategy::TimestampIntegerBased:
		return GetWorld()->TimeSeconds >= LastFireTimestamp + SecondsBetweenEachShot;
	}
}

void UBallisticWeaponComponent::SetFiringStrategy(EBallisticWeaponFiringStrategy NewStrategy)
{
	check(NewStrategy != EBallisticWeaponFiringStrategy::TimestampIntegerBased);
	FiringStrategy = NewStrategy;
	if (NewStrategy == EBallisticWeaponFiringStrategy::TimestampWithAccumulator)
	{
		MissedShotsThisFrame = 0;
	}
}

void UBallisticWeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                              FActorComponentTickFunction* ThisTickFunction)
{
	SCOPE_CYCLE_COUNTER(STAT_WeaponSystem_Tick);
	TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL_STR("Ballistic Weapon Tick", WeaponSystemChannel);

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
			StatusNotificationQueue.NotifyOnReloadRequested |= 1;
			StatusNotificationQueue.NotifyOnFiringStopped |= Status == EBallisticWeaponStatus::Firing;
		}
	}
	else if (Status == EBallisticWeaponStatus::Reloading && GetWorld()->TimeSeconds >= ReloadTimestamp)
	{
		ReloadMagazine();
		Status = HasEnoughAmmoToFire() ? EBallisticWeaponStatus::Ready : EBallisticWeaponStatus::WaitingReload;
		StatusNotificationQueue.NotifyOnReloadRequested |= Status == EBallisticWeaponStatus::WaitingReload;
	}

	NotifyStatusUpdate();
}

void UBallisticWeaponComponent::Fire()
{
	SCOPE_CYCLE_COUNTER(STAT_WeaponSystem_Fire);
	TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL_STR("Ballistic Weapon Fire", WeaponSystemChannel);

	UWorld* World = GetWorld();
	checkf(World, TEXT("Tried to fire a weapon without a World available."));
	const FVector MuzzleLocation = GetComponentLocation();
	const FVector MuzzleDirection = GetForwardVector();

	int ShotsToFire = 1;
	const double Now = World->TimeSeconds;
	if (FiringStrategy == EBallisticWeaponFiringStrategy::TimestampWithAccumulator)
	{
		const double Delta = Now - LastFireTimestamp;
		MissedShotsThisFrame += Delta > 1.0 ? Delta - 1.0 : 0.0;
		ShotsToFire += FMath::Floor(MissedShotsThisFrame);
		MissedShotsThisFrame -= ShotsToFire - 1;
	}

	LastFireTimestamp = Now;

	if (AmmoType.IsHitScan)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL_STR("Ballistic Weapon HitScan", WeaponSystemChannel);

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
				AActor* Owner = GetOwner();
				const FDamageEvent DamageEvent{AmmoType.DamageType};
				const float DamageValue = AmmoType.Damage.GetValueAtLevel(HitResult.Distance);

				// ReSharper disable CppDefaultCaseNotHandledInSwitchStatement
				switch (ShotsToFire)
				// ReSharper restore CppDefaultCaseNotHandledInSwitchStatement
				{
				case 2:
					ActorHit->TakeDamage(DamageValue, DamageEvent, Owner->GetInstigatorController(), Owner);
				case 1:
					ActorHit->TakeDamage(DamageValue, DamageEvent, Owner->GetInstigatorController(), Owner);
					break;
				}
			}
		}
	}
	else
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL_STR("Ballistic Weapon Projectile", WeaponSystemChannel);

#if !UE_BUILD_SHIPPING
		if (!AmmoType.ProjectileClass)
		{
			UE_LOGFMT(LogWeaponSystem, Error, "AmmoType can't fire a projectile if ProjectileClass isn't set!");
		}
#endif
		check(AmmoType.ProjectileClass);

		FTransform ProjectileTransform;
		ProjectileTransform.SetLocation(MuzzleLocation);
		ProjectileTransform.SetRotation(MuzzleDirection.ToOrientationQuat());
		
		FActorSpawnParameters SpawnParameters{};
		SpawnParameters.Owner = GetOwner();
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		UActorPoolSubsystem::SpawnOrAcquireFromPool(this, AmmoType.ProjectileClass, ProjectileTransform,
		                                            SpawnParameters);
	}

	StatusNotificationQueue.NotifyOnShotFired |= 1;

	if (IsBurstFire)
	{
		if (--CurrentBurstFiringCount <= 0 && Status == EBallisticWeaponStatus::Firing)
		{
			Status = HasEnoughAmmoToFire() ? EBallisticWeaponStatus::Ready : EBallisticWeaponStatus::WaitingReload;
			StatusNotificationQueue.NotifyOnFiringStopped |= 1;
			StatusNotificationQueue.NotifyOnReloadRequested |= Status == EBallisticWeaponStatus::WaitingReload;
		}
	}
}

void UBallisticWeaponComponent::UpdateMagazineAfterFiring()
{
	if (LIKELY(!HasInfiniteAmmo))
	{
		SCOPE_CYCLE_COUNTER(STAT_WeaponSystem_UpdateMagazine);
		TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL_STR("Ballistic Weapon Update Mag", WeaponSystemChannel);
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
			StatusNotificationQueue.NotifyOnFiringStopped |= Status == EBallisticWeaponStatus::Firing;
			Status = EBallisticWeaponStatus::WaitingReload;
			StatusNotificationQueue.NotifyOnReloadRequested |= 1;
		}
	}
}

void UBallisticWeaponComponent::ReloadMagazine()
{
	TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL_STR("Ballistic Weapon Reload", WeaponSystemChannel);

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
			StatusNotificationQueue.NotifyOnReloadCompleted |= 1;
		}
		else
		{
			StatusNotificationQueue.NotifyOnReloadFailed |= 1;
		}
	}
	else
	{
		CurrentMagazine = MagazineSize;
		StatusNotificationQueue.NotifyOnReloadCompleted |= 1;
	}
}

void UBallisticWeaponComponent::NotifyStatusUpdate()
{
	if (!HasPendingNotifications())
	{
		return;
	}

	SCOPE_CYCLE_COUNTER(STAT_WeaponSystem_Notify);
	TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL_STR("Ballistic Weapon Notify", WeaponSystemChannel);

	if (OnStatusChanged.IsBound())
	{
		OnStatusChanged.Broadcast(Status);
	}

	if (StatusNotificationQueue.NotifyOnReloadRequested && OnReloadRequested.IsBound())
	{
		OnReloadRequested.Broadcast();
	}
	if (StatusNotificationQueue.NotifyOnReloadStarted && OnReloadStarted.IsBound())
	{
		OnReloadStarted.Broadcast();
	}
	if (StatusNotificationQueue.NotifyOnReloadCanceled && OnReloadCanceled.IsBound())
	{
		OnReloadCanceled.Broadcast();
	}
	if (StatusNotificationQueue.NotifyOnReloadCompleted && OnReloadCompleted.IsBound())
	{
		OnReloadCompleted.Broadcast();
	}
	if (StatusNotificationQueue.NotifyOnReloadFailed && OnReloadFailed.IsBound())
	{
		OnReloadFailed.Broadcast();
	}
	if (StatusNotificationQueue.NotifyOnFiringStarted && OnFiringStarted.IsBound())
	{
		OnFiringStarted.Broadcast();
	}
	if (StatusNotificationQueue.NotifyOnFiringStopped && OnFiringStopped.IsBound())
	{
		OnFiringStopped.Broadcast();
	}
	if (StatusNotificationQueue.NotifyOnShotFired && OnShotFired.IsBound())
	{
		OnShotFired.Broadcast();
	}

	StatusNotificationQueue = {};
}
