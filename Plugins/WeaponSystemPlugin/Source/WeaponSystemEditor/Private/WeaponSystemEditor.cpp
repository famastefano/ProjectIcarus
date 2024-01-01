#include "WeaponSystemEditor.h"

#define LOCTEXT_NAMESPACE "FWeaponSystemEditorModule"

void FWeaponSystemEditorModule::StartupModule()
{
	DamageFalloffCurve_AssetTypeActions = MakeShared<FDamageFalloffCurve_AssetTypeActions>();
	FAssetToolsModule::GetModule().Get().RegisterAssetTypeActions(DamageFalloffCurve_AssetTypeActions.ToSharedRef());
}

void FWeaponSystemEditorModule::ShutdownModule()
{
	if (!FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		return;
	}
	FAssetToolsModule::GetModule().Get().UnregisterAssetTypeActions(DamageFalloffCurve_AssetTypeActions.ToSharedRef());
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FWeaponSystemEditorModule, WeaponSystemEditor)
