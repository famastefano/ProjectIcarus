#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "DamageFalloffCurve_AssetTypeActions.h"

class FWeaponSystemEditorModule : public IModuleInterface
{
	TSharedPtr<FDamageFalloffCurve_AssetTypeActions> DamageFalloffCurve_AssetTypeActions;

public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
