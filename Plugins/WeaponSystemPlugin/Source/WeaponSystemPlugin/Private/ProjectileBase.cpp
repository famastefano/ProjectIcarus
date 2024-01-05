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

void AProjectileBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void AProjectileBase::BeginPlay()
{
	if (EnableSpawnOffsetToAvoidWeaponCollision)
	{
		const double Offset = ProjectileMeshComponent->Bounds.SphereRadius + 1.f;
		const FVector InitialLocation = GetActorLocation();
		const FVector NewLocation = InitialLocation + GetActorForwardVector() * Offset;
		SetActorLocation(NewLocation, false, nullptr, ETeleportType::TeleportPhysics);
	}
	SpawnLocation = GetActorLocation();
	UE_LOGFMT(LogWeaponSystem, Display, "BeginPlay {Vector}", SpawnLocation.ToString());
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
	}

	if (ShouldDestroyAfterOverlap())
	{
		UActorPoolSubsystem::DestroyOrReleaseToPool(GetWorld(), this);
	}
}
