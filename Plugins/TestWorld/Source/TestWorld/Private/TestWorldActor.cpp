// Stefano Famà (famastefano@gmail.com)

#include "TestWorldActor.h"

ATestWorldActor::ATestWorldActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
}
