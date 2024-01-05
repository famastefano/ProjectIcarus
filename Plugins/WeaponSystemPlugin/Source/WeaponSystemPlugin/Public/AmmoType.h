// Stefano Famà (famastefano@gmail.com)

#pragma once

#include "CoreMinimal.h"
#include "ProjectileBase.h"
#include "ScalableFloat.h"
#include "AmmoType.generated.h"

/**
 * Defines which ammunition a weapon uses.
 */
USTRUCT(Blueprintable)
struct WEAPONSYSTEMPLUGIN_API FAmmoType
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ammunition")
	bool IsHitScan = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ammunition", meta=(EditCondition="!IsHitScan"))
	TSubclassOf<AProjectileBase> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ammunition", meta=(EditCondition="IsHitScan"))
	TEnumAsByte<ECollisionChannel> CollisionChannel = TEnumAsByte(ECC_Visibility);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage", meta=(EditCondition="IsHitScan"))
	TSubclassOf<UDamageType> DamageType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage", meta=(EditCondition="IsHitScan"))
	FScalableFloat Damage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage",
		meta=(EditCondition="IsHitScan", UIMin=1, ClampMin=1, Units="Meters"))
	float MaximumDistance = 1;
};
