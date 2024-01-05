// Stefano Famà (famastefano@gmail.com)

#include "ProjectileBase.h"

#include "Engine/DamageEvents.h"

#include "ActorPoolSubsystem.h"

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

bool AProjectileBase::ShouldDestroyAfterOverlap_Implementation()
{
	return true;
}

void AProjectileBase::OnActorOverlap_Implementation(AActor* OverlappedActor, AActor* OtherActor)
{
	if (OtherActor->CanBeDamaged())
	{
		const FVector TargetLocation = OtherActor->GetActorLocation();
		const double Distance = FVector::Distance(SpawnLocation, TargetLocation);
		const double DamageValue = Damage.GetValueAtLevel(Distance);

		const FDamageEvent DamageEvent{DamageType};
		OtherActor->TakeDamage(DamageValue, DamageEvent, Owner->GetInstigatorController(), Owner);

		if (ShouldDestroyAfterOverlap())
		{
			UActorPoolSubsystem::DestroyOrReleaseToPool(this, this);
		}
	}
}
