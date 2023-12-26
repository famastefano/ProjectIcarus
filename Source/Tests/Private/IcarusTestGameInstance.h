// Stefano Famà (famastefano@gmail.com)

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "IcarusTestGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class TESTS_API UIcarusTestGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	void InitForTest(UWorld& World);
	virtual TSubclassOf<AGameModeBase> OverrideGameModeClass(TSubclassOf<AGameModeBase> GameModeClass,
	                                                         const FString& MapName, const FString& Options,
	                                                         const FString& Portal) const override;
};
