// Stefano Famà (famastefano@gmail.com)

#include "IcarusPlayerController.h"

#include "IcarusCheatManager.h"

AIcarusPlayerController::AIcarusPlayerController()
{
#if !UE_BUILD_SHIPPING
	CheatClass = UIcarusCheatManager::StaticClass();
#endif
}
