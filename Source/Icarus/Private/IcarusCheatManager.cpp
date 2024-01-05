// Stefano Famà (famastefano@gmail.com)

#include "IcarusCheatManager.h"

#include "ActorPoolSubsystem.h"

void UIcarusCheatManager::EmptyAllPools() const
{
	UActorPoolSubsystem::EmptyPools(this);
}
