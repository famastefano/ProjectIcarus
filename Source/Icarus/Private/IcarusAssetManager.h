// Stefano Famà (famastefano@gmail.com)

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "IcarusAssetManager.generated.h"

UCLASS()
class ICARUS_API UIcarusAssetManager : public UAssetManager
{
	GENERATED_BODY()

public:
	static UIcarusAssetManager& Get();

	virtual void StartInitialLoading() override;
};
