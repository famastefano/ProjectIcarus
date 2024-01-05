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

void UActorPoolSubsystem::DestroyOrReleaseToPool(const UObject* WorldContextObject, AActor* Actor)
{
	checkf(Actor, TEXT("Tried to insert nullptr to the pool."));
	
	if (LIKELY(IsPoolingEnabled()))
	{
		Actor->RouteEndPlay(EEndPlayReason::Destroyed);
		UActorPoolSubsystem* Subsystem = WorldContextObject->GetWorld()->GetSubsystem<UActorPoolSubsystem>();
		auto& [Pool] = Subsystem->Pools.FindOrAdd(Actor->GetClass());
#if !UE_BUILD_SHIPPING
		checkf(!Pool.Contains(Actor), TEXT("Actor already released to the pool!"));
#endif
		Pool.Add(Actor);
	}
	else
	{
		Actor->Destroy();
	}
}

void UActorPoolSubsystem::EmptyPool(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass)
{
	check(WorldContextObject);
	UActorPoolSubsystem* Subsystem = WorldContextObject->GetWorld()->GetSubsystem<UActorPoolSubsystem>();
	if (FActorPool* Pool = Subsystem->Pools.Find(ActorClass))
	{
		for (AActor* Actor : Pool->FreeActors)
		{
			Actor->Destroy();
		}
		Subsystem->Pools.Remove(ActorClass);
	}
}

void UActorPoolSubsystem::EmptyPools(const UObject* WorldContextObject)
{
	check(WorldContextObject);
	UActorPoolSubsystem* Subsystem = WorldContextObject->GetWorld()->GetSubsystem<UActorPoolSubsystem>();
	for (auto& [Class, Pool] : Subsystem->Pools)
	{
		for (AActor* Actor : Pool.FreeActors)
		{
			Actor->Destroy();
		}
	}
	Subsystem->Pools.Empty();
}

TArray<FPoolStats> UActorPoolSubsystem::GetAllPoolStats(const UObject* WorldContextObject)
{
	check(WorldContextObject);
	UActorPoolSubsystem* Subsystem = WorldContextObject->GetWorld()->GetSubsystem<UActorPoolSubsystem>();
	TArray<FPoolStats> PoolStatistics;
	PoolStatistics.Reserve(Subsystem->Pools.Num());
	for (const auto& [Class, Pool] : Subsystem->Pools)
	{
		FPoolStats Stats;
		Stats.TypeClass = Class;
		Stats.NumberOfPooledObjects = Pool.FreeActors.Num();
		Stats.TotalPoolCapacity = Pool.FreeActors.GetSlack() + Stats.NumberOfPooledObjects;
		Stats.TotalPoolAllocatedSize = Pool.FreeActors.GetAllocatedSize();
		PoolStatistics.Add(Stats);
	}
	return PoolStatistics;
}

FPoolStats UActorPoolSubsystem::GetPoolStats(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass)
{
	check(WorldContextObject);
	check(ActorClass);
	UActorPoolSubsystem* Subsystem = WorldContextObject->GetWorld()->GetSubsystem<UActorPoolSubsystem>();
	if (FActorPool* Pool = Subsystem->Pools.Find(ActorClass))
	{
		FPoolStats Stats;
		Stats.TypeClass = ActorClass;
		Stats.NumberOfPooledObjects = Pool->FreeActors.Num();
		Stats.TotalPoolCapacity = Pool->FreeActors.GetSlack() + Stats.NumberOfPooledObjects;
		Stats.TotalPoolAllocatedSize = Pool->FreeActors.GetAllocatedSize();
		return Stats;
	}
	return {};
}
