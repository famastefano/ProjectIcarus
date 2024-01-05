// Stefano Famà (famastefano@gmail.com)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "IcarusCheatManager.generated.h"

UCLASS()
class ICARUS_API UIcarusCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	UFUNCTION(Exec)
	void EmptyAllPools() const;
};
