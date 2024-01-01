// Stefano Famà (famastefano@gmail.com)


#include "DamageFalloffCurve_AssetTypeActions.h"
#include "DamageFalloffCurve.h"

UClass* FDamageFalloffCurve_AssetTypeActions::GetSupportedClass() const
{
	return UDamageFalloffCurve::StaticClass();
}

FText FDamageFalloffCurve_AssetTypeActions::GetName() const
{
	return FText::FromString("Damage Falloff Curve");
}

FColor FDamageFalloffCurve_AssetTypeActions::GetTypeColor() const
{
	return FColor::Red;
}

uint32 FDamageFalloffCurve_AssetTypeActions::GetCategories()
{
	return EAssetTypeCategories::Misc;
}
