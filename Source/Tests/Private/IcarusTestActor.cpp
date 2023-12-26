// Stefano Famà (famastefano@gmail.com)

#include "IcarusTestActor.h"

AIcarusTestActor::AIcarusTestActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
}
