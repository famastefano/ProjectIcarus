// Stefano Famà (famastefano@gmail.com)

#pragma once

#include "CoreMinimal.h"
#include "ScalableFloat.h"

#include "GameFramework/Actor.h"
#include "GameFramework/ProjectileMovementComponent.h"

#include "ProjectileBase.generated.h"

UCLASS(Abstract, Blueprintable, BlueprintType)
class WEAPONSYSTEMPLUGIN_API AProjectileBase : public AActor
{
	GENERATED_BODY()

public:
	AProjectileBase();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> ProjectileMeshComponent;

	UPROPERTY(EditDefaultsOnly, Category="Collision")
	bool EnableSpawnOffsetToAvoidWeaponCollision = true;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Damage")
	FScalableFloat Damage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Damage")
	TSubclassOf<UDamageType> DamageType;

	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Debug")
	bool ShouldLogHits = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Debug")
	bool ShouldTraceLineEachHit = false;
#endif

protected:
	UPROPERTY(BlueprintReadOnly)
	FVector SpawnLocation;

	UFUNCTION(BlueprintNativeEvent)
	void OnActorOverlap(AActor* OverlappedActor, AActor* OtherActor);

	UFUNCTION(BlueprintNativeEvent)
	bool ShouldDestroyAfterOverlap();

	UFUNCTION(BlueprintNativeEvent)
	float CalculateDamage(double TotalTraveledDistance, AActor* ActorToDamage);
};
