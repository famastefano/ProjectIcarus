// Stefano Famà (famastefano@gmail.com)

#pragma once

#include "CoreMinimal.h"
#include "ScalableFloat.h"

#include "GameFramework/Actor.h"
#include "ProjectileBase.generated.h"

UCLASS(Abstract, Blueprintable, BlueprintType)
class WEAPONSYSTEMPLUGIN_API AProjectileBase : public AActor
{
	GENERATED_BODY()

public:
	AProjectileBase();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Damage")
	FScalableFloat Damage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Damage")
	TSubclassOf<UDamageType> DamageType;

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
