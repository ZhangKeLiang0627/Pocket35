#include "pocket35.h"
#include <mooncake.h>
#include "hal/hal.h"
#ifndef ESP_PLATFORM
#include "hal/hal_simulator/hal_simulator.hpp"
#else
#include "hal/hal_board/hal_board.h"
#endif
#include "apps/apps.h"

namespace
{
    POCKET35_APPS::AppIDs_t appIDs;
}

void POCKET35::Setup()
{
#ifndef ESP_PLATFORM
    HAL::Inject(new HAL_Simulator);
#else
    HAL::Inject(new HAL_Board);
#endif

    auto& mooncake = mooncake::GetMooncake();
    appIDs = POCKET35_APPS::installApps(mooncake);
    mooncake.update();
    mooncake.openApp(appIDs.launcher);
}

void POCKET35::Loop()
{
    HAL::UpdateTouch();
    mooncake::GetMooncake().update();
}

void POCKET35::Destroy()
{
    mooncake::DestroyMooncake();
    HAL::Destroy();
}
