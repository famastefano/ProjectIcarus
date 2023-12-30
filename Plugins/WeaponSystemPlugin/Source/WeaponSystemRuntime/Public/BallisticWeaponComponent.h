// Stefano Famà (famastefano@gmail.com)

#pragma once

#include "CoreMinimal.h"
#include "AmmoType.h"
#include "Components/SceneComponent.h"
#include "BallisticWeaponComponent.generated.h"

class UArrowComponent;

UENUM(Blueprintable)
enum class EBallisticWeaponStatus
{
	// Ready to fire.
	Ready,
	// Tick: Firing each time the RPM allows us to.
	Firing,
	// We asked to reload, won't fire until Reload has been called.
	WaitingReload,
	// Tick: Will reload once the time has been passed
	Reloading,
};

UENUM(BlueprintType)
enum class EBallisticWeaponFiringStrategy : uint8
{
	// Let the implementation decide which one is better.
	// Warning: Strategy is chosen at BeginPlay,
	// will *NOT* change automatically anymore, ie. by calling SetFireRate()
	Automatic,

	// Precise for fire-rates up to 1200 RPM
	Timestamp,

	// More precise for fire-rates over 1400 RPM
	TimestampWithAccumulator,

	// EXPERIMENTAL, DO NOT USE
	TimestampIntegerBased,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWeaponComponentBasicDelegateSignature);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWeaponComponentNotifyStatusChangeSignature, EBallisticWeaponStatus, Status)
;

/**
 * Adds ballistic weapon functionalities to an Actor:
 * - Custom fire rate RPM
 * - Ammo handling
 * - Firing via hit-scan or by spawning a projectile
 * - Reloading with precise timing
 * - Event based notification through delegates:
 *		- firing started/stopped
 *		- A shot has been fired, so ammunition have been updated
 *		- Reloading requested/started/completed
 *		- Status changed
 * The orientation of this component is considered the Muzzle where the ammo will be fired from.
 * This component requires ticking to handle firing and reloading.
 */
UCLASS(ClassGroup=("Weapon Components"), meta=(BlueprintSpawnableComponent))
class WEAPONSYSTEMRUNTIME_API UBallisticWeaponComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UBallisticWeaponComponent();

	virtual void BeginPlay() override;

#pragma region Properties

	// Never needs to reload, never consumes ammo.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
	bool HasInfiniteAmmo = false;

	// Never consumes ammo, requires reloading each magazine
	// WARNING: if AmmoUsedEachShot > CurrentMagazine, the weapon will not fire.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
	bool HasInfiniteAmmoReserve = false;

	// If true, reloading discards all the ammo left in the magazine,
	// otherwise only the ammo necessary to fill it are taken from the reserve.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
	bool ReloadingDiscardsEntireMagazine = false;

	// How much ammo a magazine has.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon", meta=(ClampMin=0, UIMin=0))
	int MagazineSize = 0;

	// Whenever the weapon fires, this amount is removed to the magazine.
	// WARNING: if AmmoUsedEachShot > CurrentMagazine, the weapon will not fire.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon", meta=(ClampMin=0, UIMin=0))
	int AmmoUsedEachShot = 1;

	// Ammunition inside the magazine.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon", meta=(ClampMin=0, UIMin=0))
	int CurrentMagazine = 0;

	// Ammo available to reload the magazine.
	// Even if ReloadingDiscardsEntireMagazine=true, this value refers to the single available ammunition, not magazines.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon", meta=(ClampMin=0, UIMin=0))
	int AmmoReserve = 0;

	// Fire rate in Rounds Per Minute.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter=SetFireRate, Category="Weapon",
		meta=(ClampMin=1, UIMin=1))
	int FireRateRpm = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter="SetFiringStrategy", Category="Weapon")
	EBallisticWeaponFiringStrategy FiringStrategy = EBallisticWeaponFiringStrategy::Automatic;

	// If the weapon shoots in burst fire mode.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
	bool IsBurstFire = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon",
		meta=(EditCondition="IsBurstFire", UIMin=1, ClampMin=1))
	int ShotsFiredDuringBurstFire = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon", meta=(UIMin=0, ClampMin=0, Units="s"))
	float SecondsToReload = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
	FAmmoType AmmoType; // TODO: Handle penetration in hit-scan?

#pragma endregion

#pragma region Delegates

	// Called whenever the weapon needs to reload.
	// Won't be able to fire again until Reload() is called. 
	UPROPERTY(BlueprintAssignable, Transient)
	FWeaponComponentBasicDelegateSignature OnReloadRequested;

	// Started reloading.
	UPROPERTY(BlueprintAssignable, Transient)
	FWeaponComponentBasicDelegateSignature OnReloadStarted;

	// Reloading canceled.
	UPROPERTY(BlueprintAssignable, Transient)
	FWeaponComponentBasicDelegateSignature OnReloadCanceled;

	// Finished reloading. Ready to fire.
	UPROPERTY(BlueprintAssignable, Transient)
	FWeaponComponentBasicDelegateSignature OnReloadCompleted;

	// Couldn't reload.
	UPROPERTY(BlueprintAssignable, Transient)
	FWeaponComponentBasicDelegateSignature OnReloadFailed;

	UPROPERTY(BlueprintAssignable, Transient)
	FWeaponComponentBasicDelegateSignature OnFiringStarted;

	// Stopped firing.
	// Possible reasons:
	// - explicitly requested via StopFiring()
	// - magazine depleted
	// - burst firing sequence completed
	UPROPERTY(BlueprintAssignable)
	FWeaponComponentBasicDelegateSignature OnFiringStopped;

	// Called each time the weapon fires.
	UPROPERTY(BlueprintAssignable)
	FWeaponComponentBasicDelegateSignature OnShotFired;

	UPROPERTY(BlueprintAssignable)
	FWeaponComponentNotifyStatusChangeSignature OnStatusChanged;

#pragma endregion

	UFUNCTION(BlueprintCallable)
	EBallisticWeaponStatus GetStatus() const;

	// Can be changed between firing.
	UFUNCTION(BlueprintCallable, BlueprintSetter, Category="Firing")
	void SetFireRate(int NewFireRateInRpm);

	// Pulls the trigger once.
	// Won't trigger Started/Stopped firing delegates.
	UFUNCTION(BlueprintCallable)
	void FireOnce();

	// Pulls the trigger and keeps it held.
	UFUNCTION(BlueprintCallable)
	void StartFiring();

	// Releases the trigger.
	UFUNCTION(BlueprintCallable)
	void StopFiring();

	// Start reloading.
	// Will reload after `SecondsToReload` seconds.
	UFUNCTION(BlueprintCallable)
	void StartReloading();

	// If reloading, cancels the operation.
	// Weapon will be ready to fire if it has enough ammo in the magazine.
	// Otherwise a reload will be requested.
	UFUNCTION(BlueprintCallable)
	void CancelReloading();

	// How many seconds will pass between one shot and the next one, based on the fire rate.
	UFUNCTION(BlueprintCallable)
	double GetSecondsBetweenShots() const;

	UFUNCTION(BlueprintCallable)
	bool HasEnoughAmmoToFire() const;

	UFUNCTION(BlueprintCallable)
	bool HasEnoughTimePassedFromLastShot() const;

	UFUNCTION(BlueprintCallable)
	void SetFiringStrategy(EBallisticWeaponFiringStrategy NewStrategy);

protected:
	void Fire();
	void UpdateMagazineAfterFiring();
	void ReloadMagazine();
	void NotifyStatusUpdate();

	double LastFireTimestamp;
	double SecondsBetweenEachShot;
	double ReloadTimestamp;
	int CurrentBurstFiringCount;

	double MissedShotsThisFrame;

	EBallisticWeaponStatus Status;

	struct alignas(uint8) FNotifyQueueFlags
	{
		uint8 NotifyOnReloadRequested : 1;
		uint8 NotifyOnReloadStarted : 1;
		uint8 NotifyOnReloadCanceled : 1;
		uint8 NotifyOnReloadCompleted : 1;
		uint8 NotifyOnReloadFailed : 1;
		uint8 NotifyOnFiringStarted : 1;
		uint8 NotifyOnFiringStopped : 1;
		uint8 NotifyOnShotFired : 1;
	} StatusNotificationQueue;

	FORCEINLINE constexpr bool HasPendingNotifications() const
	{
		static_assert(sizeof(StatusNotificationQueue) == sizeof(uint8));
		return std::bit_cast<uint8>(StatusNotificationQueue) != 0;
	}

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

private:
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UArrowComponent> BulletExitDirection;
#endif
};
