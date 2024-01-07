// Stefano Famà (famastefano@gmail.com)


#include "IcarusAssetManager.h"
#include "AbilitySystemGlobals.h"

UIcarusAssetManager& UIcarusAssetManager::Get()
{
	UIcarusAssetManager* Instance = Cast<UIcarusAssetManager>(GEngine->AssetManager);
	check(Instance);
	return *Instance;
}

void UIcarusAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();

	UAbilitySystemGlobals& AbilitySystemGlobals = UAbilitySystemGlobals::Get();
	AbilitySystemGlobals.InitGlobalData();
	AbilitySystemGlobals.InitGlobalTags();
	AbilitySystemGlobals.InitTargetDataScriptStructCache();
}
