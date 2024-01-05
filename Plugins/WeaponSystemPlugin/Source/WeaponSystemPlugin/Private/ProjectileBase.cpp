// Stefano Famà (famastefano@gmail.com)

#include "ProjectileBase.h"

#include "Engine/DamageEvents.h"

#include "ActorPoolSubsystem.h"
#include "LogWeaponSystem.h"

#include "Logging/StructuredLog.h"

#include "DrawDebugHelpers.h"

AProjectileBase::AProjectileBase()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	PrimaryActorTick.TickInterval = 0;

	if (!RootComponent)
	{
		RootComponent = CreateDefaultSubobject<USceneComponent>("RootComponent");
	}
}

void AProjectileBase::BeginPlay()
{
	SpawnLocation = GetActorLocation();
	OnActorBeginOverlap.AddDynamic(this, &AProjectileBase::OnActorOverlap);
	Super::BeginPlay();
}

void AProjectileBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	OnActorBeginOverlap.RemoveDynamic(this, &AProjectileBase::OnActorOverlap);
	Super::EndPlay(EndPlayReason);
}

float AProjectileBase::CalculateDamage_Implementation(double TotalTraveledDistance, AActor* ActorToDamage)
{
	return Damage.GetValueAtLevel(TotalTraveledDistance);
}

bool AProjectileBase::ShouldDestroyAfterOverlap_Implementation()
{
	return true;
}

void AProjectileBase::OnActorOverlap_Implementation(AActor* OverlappedActor, AActor* OtherActor)
{
#if WITH_EDITOR
	if (ShouldLogHits)
	{
		UE_LOGFMT(LogWeaponSystem, Display, "{Name} hit!", OtherActor->GetName());
	}

	if (ShouldTraceLineEachHit)
	{
		DrawDebugLine(GetWorld(), SpawnLocation, OtherActor->GetActorLocation(),
		              FColor::Red, false, 1.f, 0, 0.5f);
	}
#endif

	if (OtherActor->CanBeDamaged())
	{
		const FVector TargetLocation = OtherActor->GetActorLocation();
		const double Distance = FVector::Distance(SpawnLocation, TargetLocation);
		const float DamageValue = CalculateDamage(Distance, OtherActor);

#if WITH_EDITOR
		if (ShouldLogHits)
		{
			UE_LOGFMT(LogWeaponSystem, Display, "{Name} will be damaged by {Damage} HP. Traveled for {Distance} units.",
			          OtherActor->GetName(),
			          DamageValue,
			          Distance
			);
		}
#endif

		const FDamageEvent DamageEvent{DamageType};
		OtherActor->TakeDamage(DamageValue, DamageEvent, Owner->GetInstigatorController(), Owner);

		if (ShouldDestroyAfterOverlap())
		{
			UActorPoolSubsystem::DestroyOrReleaseToPool(this, this);
		}
	}
}
