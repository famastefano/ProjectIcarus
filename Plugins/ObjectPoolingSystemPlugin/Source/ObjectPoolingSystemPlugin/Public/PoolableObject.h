// Stefano Famà (famastefano@gmail.com)

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PoolableObject.generated.h"

UINTERFACE(MinimalAPI)
class UPoolableObject : public UInterface
{
	GENERATED_BODY()
};

/**
 * Any poolable UObject shall inherit this interface to be notified when they are transfered from/to the pool.
 * For AActors, BeginPlay and EndPlay will be used to avoid rewriting additional code.
 */
class OBJECTPOOLINGSYSTEMPLUGIN_API IPoolableObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent)
	void AcquiredFromPool();

	UFUNCTION(BlueprintImplementableEvent)
	void ReleasedToPool();
};
