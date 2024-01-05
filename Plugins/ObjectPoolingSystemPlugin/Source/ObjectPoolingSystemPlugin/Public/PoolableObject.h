// Stefano Famà (famastefano@gmail.com)

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PoolableObject.generated.h"

UINTERFACE()
class UPoolableObject : public UInterface
{
	GENERATED_BODY()
};

/**
 * Any poolable UObject shall inherit this interface to be notified when they are transferred from/to the pool.
 * For AActors, BeginPlay and EndPlay will be used to avoid rewriting additional code.
 */
class OBJECTPOOLINGSYSTEMPLUGIN_API IPoolableObject
{
	GENERATED_BODY()

public:
	virtual void AcquiredFromPool()
	{
	}

	virtual void ReleasedToPool()
	{
	}
};
