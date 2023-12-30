// Stefano Famà (famastefano@gmail.com)

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "TestWorldGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class TESTWORLD_API UTestWorldGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	void InitForTest(UWorld& World);
	virtual TSubclassOf<AGameModeBase> OverrideGameModeClass(TSubclassOf<AGameModeBase> GameModeClass,
	                                                         const FString& MapName, const FString& Options,
	                                                         const FString& Portal) const override;
};
