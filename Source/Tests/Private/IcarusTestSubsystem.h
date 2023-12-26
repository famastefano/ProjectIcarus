// Stefano Famà (famastefano@gmail.com)

#pragma once

#include "CoreMinimal.h"
#include "IcarusTestWorldHelper.h"

#include "Subsystems/EngineSubsystem.h"
#include "IcarusTestSubsystem.generated.h"

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
class TESTS_API UIcarusTestSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

	TMap<FName, FTestWorldData> PrivateWorlds;

	FTestWorldData SharedWorld;

	static FTestWorldData MakeTestWorld(FName Name);

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	FIcarusTestWorldHelper GetPrivateWorld(FName Name);
	FIcarusTestWorldHelper GetSharedWorld();

	void DestroyPrivateWorld(FName Name);
};
