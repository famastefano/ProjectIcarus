// Stefano Famà (famastefano@gmail.com)

#pragma once

#include "CoreMinimal.h"
#include "TestWorldHelper.h"

#include "Subsystems/EngineSubsystem.h"
#include "TestWorldSubsystem.generated.h"

USTRUCT()
struct FTestWorldData
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UGameInstance> GameInstance;

	UPROPERTY()
	TObjectPtr<UWorld> World;
};

UCLASS()
class TESTWORLD_API UTestWorldSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

	TMap<FName, FTestWorldData> PrivateWorlds;

	FTestWorldData SharedWorld;

	static FTestWorldData MakeTestWorld(FName Name);

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	FTestWorldHelper GetPrivateWorld(FName Name);
	FTestWorldHelper GetSharedWorld();

	void DestroyPrivateWorld(FName Name);
};
