// Stefano Famà (famastefano@gmail.com)

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "BallisticWeaponComponent.h"
#include "BallisticWeaponComponentDelegateHandler.generated.h"

UCLASS()
class TESTS_API UBallisticWeaponComponentDelegateHandler : public UObject
{
	GENERATED_BODY()

	EBallisticWeaponStatus Status{};
	
public:
	int OnReloadRequestedCounter = 0;
	int OnReloadStartedCounter = 0;
	int OnReloadCanceledCounter = 0;
	int OnReloadCompletedCounter = 0;
	int OnShotFiredCounter = 0;

	bool StatusUpdated = false;

	UFUNCTION()
	void OnReloadRequested();

	UFUNCTION()
	void OnReloadStarted();

	UFUNCTION()
	void OnReloadCanceled();

	UFUNCTION()
	void OnReloadCompleted();

	UFUNCTION()
	void OnShotFired();

	UFUNCTION()
	void OnStatusChanged(EBallisticWeaponStatus NewStatus);

	void Register(UBallisticWeaponComponent* Component);
	void UnRegister(UBallisticWeaponComponent* Component);
	void ResetCounters();

	EBallisticWeaponStatus ReadStatus();
};
