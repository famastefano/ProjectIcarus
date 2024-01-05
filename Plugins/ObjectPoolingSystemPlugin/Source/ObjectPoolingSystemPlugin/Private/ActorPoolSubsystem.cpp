// Stefano Famà (famastefano@gmail.com)


#include "ActorPoolSubsystem.h"

#include "LogObjectPoolingSystem.h"

#include "Logging/StructuredLog.h"

static TAutoConsoleVariable CVarActorPoolingEnabled(
	TEXT("ObjectPoolingSystem.ActorPooling"),
	true,
	TEXT("Enable Actor pooling for this World."),
	ECVF_Default);

static TAutoConsoleVariable CVarActorPoolingEnableLogging(
	TEXT("ObjectPoolingSystem.EnableActorLogging"),
	false,
	TEXT("Enable logging when using the Actor Pooling System"),
	ECVF_Default
);

static const FAutoConsoleCommandWithWorld CVarActorPoolingEmptyPools(
	TEXT("ObjectPoolingSystem.EmptyPools"),
	TEXT("Empties all the pools."),
	FConsoleCommandWithWorldDelegate::CreateStatic(&UActorPoolSubsystem::EmptyPools)
);

static const FAutoConsoleCommandWithWorld CVarActorPoolingLogStats(
	TEXT("ObjectPoolingSystem.LogStats"),
	TEXT("Logs the statistics of all pools."),
	FConsoleCommandWithWorldDelegate::CreateStatic(&UActorPoolSubsystem::LogStats)
);

bool UActorPoolSubsystem::IsPoolingEnabled()
{
	return CVarActorPoolingEnabled.GetValueOnAnyThread();
}

bool UActorPoolSubsystem::IsLoggingEnabled()
{
	return CVarActorPoolingEnableLogging.GetValueOnAnyThread();
}

void UActorPoolSubsystem::PopulatePool(UWorld* World, TSubclassOf<AActor> ActorClass, int32 Count)
{
	check(World);
	check(ActorClass);
	check(Count > 0);

#if !UE_BUILD_SHIPPING
	if (!IsPoolingEnabled())
	{
		UE_LOGFMT(LogObjectPoolingSystem, Warning,
		          "Object pooling is disabled in World {Name}, but you asked to populate the pool with {Count} Actors of class {Class}."
		          ,
		          World->GetName(),
		          Count,
		          ActorClass->GetName());
	}
#endif

	if (UNLIKELY(!IsPoolingEnabled()))
	{
		return;
	}

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

	UActorPoolSubsystem* Subsystem = World->GetSubsystem<UActorPoolSubsystem>();
	Subsystem->Pools.FindOrAdd(ActorClass).FreeActors.Append(PooledActors);

	if (IsLoggingEnabled())
	{
		UE_LOGFMT(LogObjectPoolingSystem, Display, "Populated Pool with {Count} Actors of Class {Class}", Count,
		          ActorClass->GetName());
	}
}

AActor* UActorPoolSubsystem::SpawnOrAcquireFromPool(
	UWorld* World,
	TSubclassOf<AActor> ActorClass,
	const FTransform& SpawnTransform,
	const FActorSpawnParameters& SpawnParams)
{
	check(World);
	check(ActorClass)

	if (LIKELY(IsPoolingEnabled()))
	{
		UActorPoolSubsystem* Subsystem = World->GetSubsystem<UActorPoolSubsystem>();
		if (FActorPool* Pool = Subsystem->Pools.Find(ActorClass); Pool && !Pool->FreeActors.IsEmpty())
		{
			AActor* Actor = Pool->FreeActors.Pop(false);

			if (IsLoggingEnabled())
			{
				UE_LOGFMT(LogObjectPoolingSystem, Display, "Reusing Actor {Name} of Class {Class}", Actor->GetName(),
				          ActorClass->GetName());
			}

			Actor->SetActorTransform(SpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);
			Actor->SetOwner(SpawnParams.Owner);
			Actor->DispatchBeginPlay();
			return Actor;
		}
	}
	AActor* Actor = World->SpawnActor<AActor>(ActorClass, SpawnTransform, SpawnParams);
	if (IsLoggingEnabled())
	{
		UE_LOGFMT(LogObjectPoolingSystem, Display, "Spawning new Actor {Name} of Class {Class}.", Actor->GetName(),
		          ActorClass->GetName());
	}
	return Actor;
}

void UActorPoolSubsystem::DestroyOrReleaseToPool(UWorld* World, AActor* Actor)
{
	check(World);
	checkf(Actor, TEXT("Tried to insert nullptr to the pool."));

	if (LIKELY(IsPoolingEnabled()))
	{
		Actor->RouteEndPlay(EEndPlayReason::Destroyed);
		UActorPoolSubsystem* Subsystem = World->GetSubsystem<UActorPoolSubsystem>();
		auto& [Pool] = Subsystem->Pools.FindOrAdd(Actor->GetClass());
		checkfSlow(!Pool.Contains(Actor), TEXT("Actor already released to the pool!"));
		Pool.Add(Actor);
		if (IsLoggingEnabled())
		{
			UE_LOGFMT(LogObjectPoolingSystem, Display, "Released Actor {Name} to the class pool.", Actor->GetName());
		}
	}
	else
	{
		if (IsLoggingEnabled())
		{
			UE_LOGFMT(LogObjectPoolingSystem, Display, "Destroyed Actor {Name} because pooling is disabled.",
			          Actor->GetName());
		}
		Actor->Destroy();
	}
}

void UActorPoolSubsystem::EmptyPool(UWorld* World, TSubclassOf<AActor> ActorClass)
{
	check(World);
	check(ActorClass);
	UActorPoolSubsystem* Subsystem = World->GetSubsystem<UActorPoolSubsystem>();
	if (FActorPool* Pool = Subsystem->Pools.Find(ActorClass))
	{
		for (AActor* Actor : Pool->FreeActors)
		{
			Actor->Destroy();
		}
		Subsystem->Pools.Remove(ActorClass);
	}
	if (IsLoggingEnabled())
	{
		UE_LOGFMT(LogObjectPoolingSystem, Display, "Emptied pool of Class {Class}.", ActorClass->GetName());
	}
}

void UActorPoolSubsystem::EmptyPools(UWorld* World)
{
	check(World);
	UActorPoolSubsystem* Subsystem = World->GetSubsystem<UActorPoolSubsystem>();
	for (auto& [Class, Pool] : Subsystem->Pools)
	{
		for (AActor* Actor : Pool.FreeActors)
		{
			Actor->Destroy();
		}
	}
	Subsystem->Pools.Empty();
	if (IsLoggingEnabled())
	{
		UE_LOGFMT(LogObjectPoolingSystem, Display, "Emptied all pools");
	}
}

TArray<FPoolStats> UActorPoolSubsystem::GetAllPoolStats(UWorld* World)
{
	check(World);
	UActorPoolSubsystem* Subsystem = World->GetSubsystem<UActorPoolSubsystem>();
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

FPoolStats UActorPoolSubsystem::GetPoolStats(UWorld* World, TSubclassOf<AActor> ActorClass)
{
	check(World);
	check(ActorClass);
	UActorPoolSubsystem* Subsystem = World->GetSubsystem<UActorPoolSubsystem>();
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

void UActorPoolSubsystem::LogStats(UWorld* World)
{
	const auto& StatsCollection = GetAllPoolStats(World);
	if (StatsCollection.IsEmpty())
	{
		UE_LOG(LogObjectPoolingSystem, Display, TEXT("No pool is present."));
		return;
	}
	for (const auto& PoolStats : StatsCollection)
	{
		UE_LOGFMT(LogObjectPoolingSystem,
		          Display,
		          "Pool {Class} contains {Count} Actors and occupies {Memory} MB.",
		          PoolStats.TypeClass->GetName(),
		          PoolStats.NumberOfPooledObjects,
		          PoolStats.TotalPoolAllocatedSize / (1024*1024)
		);
	}
}
