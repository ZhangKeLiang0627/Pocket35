#pragma once
#include <mooncake.h>
#include "launcher/launcher.h"
#include "app_link/app_link.h"
#include "app_template/app_template.h"

namespace POCKET35_APPS
{
    struct AppIDs_t
    {
        int launcher = -1;
        int link = -1;
        int appTemplate = -1;
    };

    inline AppIDs_t installApps(mooncake::Mooncake& mooncake)
    {
        AppIDs_t ids;

        ids.launcher = mooncake.installApp(std::make_unique<MOONCAKE::APPS::AppLauncher>());
        ids.link = mooncake.installApp(std::make_unique<MOONCAKE::APPS::AppLink>());
        ids.appTemplate = mooncake.installApp(std::make_unique<MOONCAKE::APPS::AppTemplate>());

        return ids;
    }
} // namespace POCKET35_APPS
