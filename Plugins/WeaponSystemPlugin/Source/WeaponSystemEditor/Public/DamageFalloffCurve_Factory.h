// Stefano Famà (famastefano@gmail.com)

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "DamageFalloffCurve_Factory.generated.h"

/**
 * 
 */
UCLASS()
class WEAPONSYSTEMEDITOR_API UDamageFalloffCurve_Factory : public UFactory
{
	GENERATED_BODY()

public:
	UDamageFalloffCurve_Factory();
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags,
	                                  UObject* Context, FFeedbackContext* Warn) override;
};
