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

	ProjectileMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("ProjectileMesh");
	ProjectileMeshComponent->SetupAttachment(RootComponent);

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>("ProjectileMovementComponent");
	ProjectileMovementComponent->UpdatedComponent = RootComponent;
}

void AProjectileBase::BeginPlay()
{
	InitialSpeed = ProjectileMovementComponent->InitialSpeed;
	IsActorHiddenAtBeginPlay = IsHidden();
	AcquiredFromPool(GetActorTransform(), GetOwner());
	Super::BeginPlay();
}

void AProjectileBase::AcquiredFromPool(const FTransform& NewTransform, AActor* NewOwner)
{
	if (EnableSpawnOffsetToAvoidWeaponCollision)
	{
		const double Offset = ProjectileMeshComponent->Bounds.SphereRadius + 1.f;
		const FVector InitialLocation = NewTransform.GetLocation();
		const FVector NewLocation = InitialLocation + NewTransform.GetRotation().GetForwardVector() * Offset;

		FTransform TransformWithOffset = NewTransform;
		TransformWithOffset.SetLocation(NewLocation);
		SetActorTransform(TransformWithOffset, false, nullptr, ETeleportType::TeleportPhysics);
	}
	else
	{
		SetActorTransform(NewTransform, false, nullptr, ETeleportType::TeleportPhysics);
	}

	SetOwner(NewOwner);
	SpawnLocation = GetActorLocation();
	OnActorBeginOverlap.AddDynamic(this, &AProjectileBase::OnActorOverlap);
	SetActorEnableCollision(true);
	SetActorHiddenInGame(IsActorHiddenAtBeginPlay);
	if (ProjectileMovementComponent->HasStoppedSimulation())
	{
		const FVector Velocity = GetActorForwardVector() * InitialSpeed;
		ProjectileMovementComponent->Velocity = ProjectileMovementComponent->ComputeVelocity(Velocity, 0);
		ProjectileMovementComponent->SetUpdatedComponent(RootComponent);
		ProjectileMovementComponent->SetActive(true);
	}
}

void AProjectileBase::ReleasedToPool()
{
	ProjectileMovementComponent->StopSimulating({});
	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);
	OnActorBeginOverlap.RemoveDynamic(this, &AProjectileBase::OnActorOverlap);
}

bool AProjectileBase::ShouldIgnoreHit(AActor* ActorHit) const
{
	return ActorHit == GetOwner()
		&& FVector::DistSquared(SpawnLocation, GetActorLocation())
		< FVector::DistSquared(SpawnLocation, ActorHit->GetActorLocation());
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
		DrawDebugLine(GetWorld(), SpawnLocation, GetActorLocation(),
		              FColor::Red, false, 1.f, 0, 0.5f);
	}
#endif

	if (ShouldIgnoreHit(OtherActor))
	{
		UE_LOGFMT(LogWeaponSystem, Display, "Ignoring hit.");
		return;
	}

	if (OtherActor->CanBeDamaged())
	{
		const FVector HitLocation = GetActorLocation();
		const double Distance = FVector::Distance(SpawnLocation, HitLocation);
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
	}

	if (ShouldDestroyAfterOverlap())
	{
		UActorPoolSubsystem::DestroyOrReleaseToPool(GetWorld(), this);
	}
}
