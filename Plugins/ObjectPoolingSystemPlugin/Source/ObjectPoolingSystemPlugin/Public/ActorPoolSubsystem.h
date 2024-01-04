// Stefano Famà (famastefano@gmail.com)

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ActorPoolSubsystem.generated.h"

USTRUCT()
struct FPooledActor
{
	GENERATED_BODY()

	UPROPERTY()
	AActor* Actor;
	bool IsActive;
};

USTRUCT()
struct FActorPool
{
	GENERATED_BODY()

	TArray<FPooledActor> Pool;
};

UCLASS()
class OBJECTPOOLINGSYSTEMPLUGIN_API UActorPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<TSubclassOf<AActor>, FActorPool> Pools;

public:
	static bool IsPoolingEnabled();

	// Populate a pool by constructing Count instances of ActorClass
	static void PopulatePool(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, int32 Count);

	// IsPoolingEnabled() = true  : if possible, reuses an existing Actor,
	//                              otherwise spawns a new one, then adds it to the pool. 
	//                    = false : always spawns a new Actor.
	static AActor* SpawnOrAcquireFromPool(const UObject* WorldContextObject,
	                                      TSubclassOf<AActor> ActorClass,
	                                      const FTransform& SpawnTransform,
	                                      const FActorSpawnParameters& SpawnParams);

	// IsPoolingEnabled() = true  : Actor->EndPlay(Destroyed), then adds it to the pool.
	//                    = false : Actor->Destroy()
	static void DestroyOrReleaseToPool(const UObject* WorldContextObject, AActor*& Actor);
};
