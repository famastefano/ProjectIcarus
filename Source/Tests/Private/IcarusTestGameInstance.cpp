// Stefano Famà (famastefano@gmail.com)

#include "IcarusTestGameInstance.h"

#include "IcarusTestGameMode.h"

void UIcarusTestGameInstance::InitForTest(UWorld& World)
{
	FWorldContext* TestWorldContext = GEngine->GetWorldContextFromWorld(&World);
	check(TestWorldContext);
	WorldContext = TestWorldContext;
	WorldContext->OwningGameInstance = this;
	World.SetGameInstance(this);
	World.SetGameMode(FURL());

	Init();
}

TSubclassOf<AGameModeBase> UIcarusTestGameInstance::OverrideGameModeClass(TSubclassOf<AGameModeBase> GameModeClass,
                                                                          const FString& MapName,
                                                                          const FString& Options,
                                                                          const FString& Portal) const
{
	return AIcarusTestGameMode::StaticClass();
}
