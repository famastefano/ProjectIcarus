// Stefano Famà (famastefano@gmail.com)

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PoolableActor.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UPoolableActor : public UInterface
{
	GENERATED_BODY()
};

/**
 * Any poolable AActor shall inherit this interface to be notified when they are transferred from/to the pool.
 * For AActors, BeginPlay and EndPlay will be used to avoid rewriting additional code.
 */
class OBJECTPOOLINGSYSTEMPLUGIN_API IPoolableActor
{
	GENERATED_BODY()

public:
	virtual void AcquiredFromPool(const FTransform& NewTransform, AActor* NewOwner)
	{
	}

	virtual void ReleasedToPool()
	{
	}
};
