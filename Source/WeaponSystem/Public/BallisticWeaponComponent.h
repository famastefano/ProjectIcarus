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
 *		- A shot has been fired, so ammunition have been updated
 *		- Reloading requested/started/completed
 *		- Status changed
 * The orientation of this component is considered the Muzzle where the ammo will be fired from.
 * This component requires ticking to handle firing and reloading.
 */
UCLASS(ClassGroup=("Weapon Components"), meta=(BlueprintSpawnableComponent))
class WEAPONSYSTEM_API UBallisticWeaponComponent : public USceneComponent
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
		meta=(ClampMin=0, UIMin=0))
	int FireRateRpm = 0;

	// If the weapon shoots in burst fire mode.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
	bool IsBurstFire = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon",
		meta=(EditCondition="IsBurstFire", UIMin=1, ClampMin=1))
	int ShotsFiredDuringBurstFire = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon", meta=(UIMin=0, ClampMin=0, Units="s"))
	float SecondsToReload = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
	FAmmoType AmmoType;

#pragma endregion

#pragma region Delegates

	// Called whenever the weapon needs to reload.
	// Won't be able to fire again until Reload() is called. 
	UPROPERTY(BlueprintAssignable)
	FWeaponComponentBasicDelegateSignature OnReloadRequested;

	// Started reloading.
	UPROPERTY(BlueprintAssignable)
	FWeaponComponentBasicDelegateSignature OnReloadStarted;

	// Reloading canceled.
	UPROPERTY(BlueprintAssignable)
	FWeaponComponentBasicDelegateSignature OnReloadCanceled;

	// Finished reloading. Ready to fire.
	UPROPERTY(BlueprintAssignable)
	FWeaponComponentBasicDelegateSignature OnReloadCompleted;

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

protected:
	void Fire();
	void ReloadMagazine();

	double LastFireTimestamp;
	double SecondsBetweenEachShot;
	double ReloadTimestamp;

	EBallisticWeaponStatus Status;

	FTraceHandle HitScanLineTraceHandle;
	FTraceDelegate OnHitDelegate;

	void OnHitScanCompleted(FTraceHandle const& TraceHandle, FTraceDatum& TraceDatum) const;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

private:
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UArrowComponent> BulletExitDirection;
#endif
};
