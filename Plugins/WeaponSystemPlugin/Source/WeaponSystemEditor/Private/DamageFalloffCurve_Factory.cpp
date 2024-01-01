// Stefano Famà (famastefano@gmail.com)

#include "DamageFalloffCurve_Factory.h"
#include "DamageFalloffCurve.h"

UDamageFalloffCurve_Factory::UDamageFalloffCurve_Factory()
{
	SupportedClass = UDamageFalloffCurve::StaticClass();
	bCreateNew = true;
}

UObject* UDamageFalloffCurve_Factory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName,
                                                       EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UDamageFalloffCurve>(InParent, InClass, InName, Flags, Context);
}
