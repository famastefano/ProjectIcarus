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
	SecondsBetweenEachShot = GetSecondsBetweenShots();
	LastFireTimestamp = -SecondsBetweenEachShot;
	Status = HasEnoughAmmoToFire() ? EBallisticWeaponStatus::Ready : EBallisticWeaponStatus::WaitingReload;
	OnHitDelegate.BindUObject(this, &UBallisticWeaponComponent::OnHitScanCompleted);
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
		UE_LOGFMT(LogWeaponSystem, Error, "Invalid new fire rate: {NewFireRate} RPM.", NewFireRateInRpm);
#endif

	FireRateRpm = NewFireRateInRpm;
	SecondsBetweenEachShot = GetSecondsBetweenShots();
}

void UBallisticWeaponComponent::FireOnce()
{
	if (Status == EBallisticWeaponStatus::Ready && HasEnoughAmmoToFire() && HasEnoughTimePassedFromLastShot())
	{
		if (IsBurstFire)
			Status = EBallisticWeaponStatus::Firing;
		Fire();
	}
}

void UBallisticWeaponComponent::StartFiring()
{
	if (Status == EBallisticWeaponStatus::Ready && HasEnoughAmmoToFire() && HasEnoughTimePassedFromLastShot())
	{
		Status = EBallisticWeaponStatus::Firing;
		Fire();
	}
}

void UBallisticWeaponComponent::StopFiring()
{
	if (Status == EBallisticWeaponStatus::Firing)
		Status = CurrentMagazine > 0 ? EBallisticWeaponStatus::Ready : EBallisticWeaponStatus::WaitingReload;
}

void UBallisticWeaponComponent::StartReloading()
{
	if (Status != EBallisticWeaponStatus::WaitingReload)
	{
		Status = EBallisticWeaponStatus::WaitingReload;
		ReloadTimestamp = GetWorld()->TimeSeconds + SecondsToReload;
		if (OnReloadStarted.IsBound())
			OnReloadStarted.Broadcast();
	}
}

void UBallisticWeaponComponent::CancelReloading()
{
	if (Status == EBallisticWeaponStatus::Reloading)
	{
		Status = HasEnoughAmmoToFire() ? EBallisticWeaponStatus::Ready : EBallisticWeaponStatus::WaitingReload;
		if (Status == EBallisticWeaponStatus::WaitingReload && OnReloadRequested.IsBound())
			OnReloadRequested.Broadcast();
	}
}

double UBallisticWeaponComponent::GetSecondsBetweenShots() const
{
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
	if (Status == EBallisticWeaponStatus::Firing)
	{
		if (HasEnoughAmmoToFire())
		{
			if (HasEnoughTimePassedFromLastShot())
				Fire();
		}
		else
		{
			Status = EBallisticWeaponStatus::WaitingReload;
			if (OnReloadRequested.IsBound())
				OnReloadRequested.Broadcast();
		}
	}
	else if (Status == EBallisticWeaponStatus::Reloading && GetWorld()->TimeSeconds >= ReloadTimestamp)
	{
		ReloadMagazine();
		if (OnReloadCompleted.IsBound())
			OnReloadCompleted.Broadcast();
	}
}

void UBallisticWeaponComponent::Fire()
{
	UWorld* World = GetWorld();
	checkf(World, TEXT("Tried to fire a weapon without a World available."));
	FVector const MuzzleLocation = GetComponentLocation();
	FVector const MuzzleDirection = GetForwardVector();

	LastFireTimestamp = World->TimeSeconds;

	if (AmmoType.IsHitScan)
	{
		// TODO: Support DamageFalloffCurve after the proper editor has been created
		constexpr float MaximumDistance = 100'000; // 1000 m

#if !UE_BUILD_SHIPPING
		DrawDebugLine(World, MuzzleLocation, MuzzleLocation + MuzzleDirection * MaximumDistance,
		              FColor::Red, false, 1.f, 0, 0.5f);
#endif

		HitScanLineTraceHandle = World->AsyncLineTraceByChannel(
			EAsyncTraceType::Single,
			MuzzleLocation,
			MuzzleLocation + MuzzleDirection * MaximumDistance,
			AmmoType.CollisionChannel,
			FCollisionQueryParams::DefaultQueryParam,
			FCollisionResponseParams::DefaultResponseParam,
			&OnHitDelegate);
	}
	else
	{
#if !UE_BUILD_SHIPPING
		if (!AmmoType.ProjectileClass)
			UE_LOGFMT(LogWeaponSystem, Error, "AmmoType can't fire a projectile if ProjectileClass isn't set!");
#endif
		// TODO: Spawn Projectile Component
	}

	if (OnShotFired.IsBound())
		OnShotFired.Broadcast();

	if (LIKELY(!HasInfiniteAmmo))
	{
		CurrentMagazine -= AmmoUsedEachShot;
#if !UE_BUILD_SHIPPING
		if (CurrentMagazine < 0)
			UE_LOGFMT(LogWeaponSystem, Error,
		          "Invalid magazine, this weapon shouldn't have fired at all. Magazine {Magazine}, Rounds per shot {RoundsPerShot}.",
		          CurrentMagazine, AmmoUsedEachShot);
#endif
		if (CurrentMagazine <= 0)
		{
			Status = EBallisticWeaponStatus::WaitingReload;
			if (OnReloadRequested.IsBound())
				OnReloadRequested.Broadcast();
		}
	}
}

void UBallisticWeaponComponent::ReloadMagazine()
{
	if (ReloadingDiscardsEntireMagazine)
		CurrentMagazine = 0;

	checkf(MagazineSize >= CurrentMagazine, TEXT("Invalid precondition: MagazineSize >= CurrentMagazine (%d >= %d)"),
	       MagazineSize, CurrentMagazine);

	if (LIKELY(!HasInfiniteAmmoReserve))
	{
		int16 const MissingRounds = MagazineSize - CurrentMagazine;
		int16 const RoundsAvailable = AmmoReserve >= MissingRounds ? MissingRounds : AmmoReserve;
		CurrentMagazine += RoundsAvailable;
		AmmoReserve -= RoundsAvailable;
	}
	else
	{
		CurrentMagazine = MagazineSize;
	}
}

void UBallisticWeaponComponent::OnHitScanCompleted(FTraceHandle const& TraceHandle, FTraceDatum& TraceDatum) const
{
	ensure(TraceHandle == HitScanLineTraceHandle);

	if (!TraceDatum.OutHits.IsEmpty())
	{
		FHitResult const& HitResult = TraceDatum.OutHits[0];

		UE_LOGFMT(LogWeaponSystem, Verbose, "Hit {Actor}, {Distance} cm far", *HitResult.GetActor()->GetName(),
		          HitResult.Distance);

		if (AActor* ActorHit = HitResult.GetActor(); ActorHit && ActorHit->CanBeDamaged())
		{
			// TODO: Calculate damage falloff based on distance
			AActor* Owner = GetOwner();
			FDamageEvent const DamageEvent{AmmoType.DamageType};
			ActorHit->TakeDamage(AmmoType.DamageAmount, DamageEvent, Owner->GetInstigatorController(), Owner);
		}
	}
}
