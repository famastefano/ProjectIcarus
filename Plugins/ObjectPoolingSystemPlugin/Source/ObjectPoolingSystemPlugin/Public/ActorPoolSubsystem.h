// Stefano Famà (famastefano@gmail.com)

#pragma once

#include "CoreMinimal.h"
#include "PoolStats.h"

#include "Subsystems/WorldSubsystem.h"
#include "ActorPoolSubsystem.generated.h"

USTRUCT()
struct FActorPool
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<AActor*> FreeActors;
};

UCLASS()
class OBJECTPOOLINGSYSTEMPLUGIN_API UActorPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<TSubclassOf<AActor>, FActorPool> Pools;

public:
	static bool IsPoolingEnabled();

	static bool IsLoggingEnabled();

	// Populate a pool by constructing Count instances of ActorClass
	static void PopulatePool(UWorld* World, TSubclassOf<AActor> ActorClass, int32 Count);

	// IsPoolingEnabled() = true  : if possible, reuses an existing Actor,
	//                              otherwise spawns a new one, then adds it to the pool. 
	//                    = false : always spawns a new Actor.
	static AActor* SpawnOrAcquireFromPool(UWorld* World,
	                                      TSubclassOf<AActor> ActorClass,
	                                      const FTransform& SpawnTransform = FTransform::Identity,
	                                      const FActorSpawnParameters& SpawnParams = {});

	// IsPoolingEnabled() = true  : Actor->EndPlay(Destroyed), then adds it to the pool.
	//                    = false : Actor->Destroy()
	static void DestroyOrReleaseToPool(UWorld* World, AActor* Actor);

	static void EmptyPool(UWorld* World, TSubclassOf<AActor> ActorClass);
	static void EmptyPools(UWorld* World);

	static TArray<FPoolStats> GetAllPoolStats(UWorld* World);
	static FPoolStats GetPoolStats(UWorld* World, TSubclassOf<AActor> ActorClass);

	static void LogStats(UWorld* World);
};
