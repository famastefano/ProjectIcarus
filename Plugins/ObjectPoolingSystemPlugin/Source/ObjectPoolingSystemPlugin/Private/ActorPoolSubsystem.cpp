// Stefano Famà (famastefano@gmail.com)


#include "ActorPoolSubsystem.h"

#include "LogObjectPoolingSystem.h"

#include "Logging/StructuredLog.h"

static TAutoConsoleVariable CVarActorPoolingEnabled(
	TEXT("ObjectPoolingSystem.ActorPooling"),
	true,
	TEXT("Enable Actor pooling for this World."),
	ECVF_Default);

bool UActorPoolSubsystem::IsPoolingEnabled()
{
	return CVarActorPoolingEnabled.GetValueOnAnyThread();
}

void UActorPoolSubsystem::PopulatePool(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, int32 Count)
{
	check(WorldContextObject);
	check(ActorClass);
	check(Count > 0);

#if !UE_BUILD_SHIPPING
	if (!IsPoolingEnabled())
	{
		UE_LOGFMT(LogObjectPoolingSystem, Warning,
		          "Object pooling is disabled in World {Name}, but you asked to populate the pool with {Count} Actors of class {Class}."
		          ,
		          WorldContextObject->GetWorld()->GetName(),
		          Count,
		          ActorClass->GetName());
	}
#endif

	if (UNLIKELY(!IsPoolingEnabled()))
	{
		return;
	}

	UWorld* World = WorldContextObject->GetWorld();

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	TArray<AActor*> PooledActors;
	PooledActors.Reserve(Count);
	for (int32 SpawnedActors = 0; SpawnedActors < Count; ++SpawnedActors)
	{
		if (AActor* Actor = World->SpawnActor<AActor>(ActorClass, FTransform::Identity, Params))
		{
			PooledActors.Add(Actor);
		}
	}

	UActorPoolSubsystem* Subsystem = WorldContextObject->GetWorld()->GetSubsystem<UActorPoolSubsystem>();
	Subsystem->Pools.FindOrAdd(ActorClass).FreeActors.Append(PooledActors);
}

AActor* UActorPoolSubsystem::SpawnOrAcquireFromPool(
	const UObject* WorldContextObject,
	TSubclassOf<AActor> ActorClass,
	const FTransform& SpawnTransform,
	const FActorSpawnParameters& SpawnParams)
{
	check(WorldContextObject);
	check(ActorClass)

	UWorld* World = WorldContextObject->GetWorld();
	check(World);

	if (LIKELY(IsPoolingEnabled()))
	{
		UActorPoolSubsystem* Subsystem = World->GetSubsystem<UActorPoolSubsystem>();
		if (FActorPool* Pool = Subsystem->Pools.Find(ActorClass); Pool && !Pool->FreeActors.IsEmpty())
		{
			AActor* Actor = Pool->FreeActors.Pop(false);
			Actor->SetActorTransform(SpawnTransform);
			Actor->SetOwner(SpawnParams.Owner);
			Actor->DispatchBeginPlay();
			return Actor;
		}
	}
	return World->SpawnActor<AActor>(ActorClass, SpawnTransform, SpawnParams);
}

void UActorPoolSubsystem::DestroyOrReleaseToPool(const UObject* WorldContextObject, AActor*& Actor)
{
	if (LIKELY(IsPoolingEnabled()))
	{
		Actor->RouteEndPlay(EEndPlayReason::Destroyed);
		UActorPoolSubsystem* Subsystem = WorldContextObject->GetWorld()->GetSubsystem<UActorPoolSubsystem>();
		auto& [Pool] = Subsystem->Pools.FindOrAdd(Actor->GetClass());
		Pool.Add(Actor);
	}
	else
	{
		Actor->Destroy();
	}
}
