﻿// Stefano Famà (famastefano@gmail.com)

#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class WEAPONSYSTEMEDITOR_API FDamageFalloffCurve_AssetTypeActions : public FAssetTypeActions_Base
{
public:
	virtual UClass* GetSupportedClass() const override;
	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;
	virtual uint32 GetCategories() override;
};
