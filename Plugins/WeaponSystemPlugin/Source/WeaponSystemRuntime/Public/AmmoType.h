// Stefano Famà (famastefano@gmail.com)

#pragma once

#include "CoreMinimal.h"
#include "AmmoType.generated.h"

/**
 * Defines which ammunition a weapon uses.
 */
USTRUCT(Blueprintable)
struct WEAPONSYSTEMRUNTIME_API FAmmoType
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ammunition")
	bool IsHitScan = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ammunition", meta=(EditCondition="!IsHitScan"))
	TSubclassOf<UObject> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ammunition", meta=(EditCondition="IsHitScan"))
	TEnumAsByte<ECollisionChannel> CollisionChannel = TEnumAsByte(ECC_Visibility);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage",
		meta=(EditCondition="IsHitScan", UIMin=0, ClampMin=0))
	float DamageAmount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage", meta=(EditCondition="IsHitScan"))
	TSubclassOf<UDamageType> DamageType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage", meta=(EditCondition="IsHitScan"))
	TObjectPtr<UCurveFloat> DamageFalloffCurve;
};
