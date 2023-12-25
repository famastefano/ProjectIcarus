// Stefano Famà (famastefano@gmail.com)


#include "BallisticWeaponComponentDelegateHandler.h"

void UBallisticWeaponComponentDelegateHandler::OnReloadRequested()
{
	++OnReloadRequestedCounter;
}

void UBallisticWeaponComponentDelegateHandler::OnReloadStarted()
{
	++OnReloadStartedCounter;
}

void UBallisticWeaponComponentDelegateHandler::OnReloadCanceled()
{
	++OnReloadCanceledCounter;
}

void UBallisticWeaponComponentDelegateHandler::OnReloadCompleted()
{
	++OnReloadCompletedCounter;
}

void UBallisticWeaponComponentDelegateHandler::OnShotFired()
{
	++OnShotFiredCounter;
}

void UBallisticWeaponComponentDelegateHandler::OnStatusChanged(EBallisticWeaponStatus NewStatus)
{
	StatusUpdated = true;
	Status = NewStatus;
}

void UBallisticWeaponComponentDelegateHandler::Register(UBallisticWeaponComponent* Component)
{
	Component->OnReloadRequested.AddDynamic(this, &UBallisticWeaponComponentDelegateHandler::OnReloadRequested);
	Component->OnReloadStarted.AddDynamic(this, &UBallisticWeaponComponentDelegateHandler::OnReloadStarted);
	Component->OnReloadCanceled.AddDynamic(this, &UBallisticWeaponComponentDelegateHandler::OnReloadCanceled);
	Component->OnReloadCompleted.AddDynamic(this, &UBallisticWeaponComponentDelegateHandler::OnReloadCompleted);
	Component->OnShotFired.AddDynamic(this, &UBallisticWeaponComponentDelegateHandler::OnShotFired);
}

void UBallisticWeaponComponentDelegateHandler::UnRegister(UBallisticWeaponComponent* Component)
{
	Component->OnReloadRequested.RemoveDynamic(this, &UBallisticWeaponComponentDelegateHandler::OnReloadRequested);
	Component->OnReloadStarted.RemoveDynamic(this, &UBallisticWeaponComponentDelegateHandler::OnReloadStarted);
	Component->OnReloadCanceled.RemoveDynamic(this, &UBallisticWeaponComponentDelegateHandler::OnReloadCanceled);
	Component->OnReloadCompleted.RemoveDynamic(this, &UBallisticWeaponComponentDelegateHandler::OnReloadCompleted);
	Component->OnShotFired.RemoveDynamic(this, &UBallisticWeaponComponentDelegateHandler::OnShotFired);
}

void UBallisticWeaponComponentDelegateHandler::ResetCounters()
{
	OnReloadRequestedCounter = 0;
	OnReloadStartedCounter = 0;
	OnReloadCanceledCounter = 0;
	OnReloadCompletedCounter = 0;
	OnShotFiredCounter = 0;
}

EBallisticWeaponStatus UBallisticWeaponComponentDelegateHandler::ReadStatus()
{
	check(StatusUpdated);
	StatusUpdated = false;
	return Status;
}
