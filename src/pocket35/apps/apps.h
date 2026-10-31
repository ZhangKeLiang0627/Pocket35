#pragma once
#include <mooncake.h>
#include "launcher/launcher.h"
#include "app_link/app_link.h"
#include "app_imu/app_imu.h"
#include "app_battery/app_battery.h"
#include "app_wifi/app_wifi.h"
#include "app_clock/app_clock.h"
#include "app_lvgl_demo/app_lvgl_demo.h"
#include "app_fluidbox/app_fluidbox.h"
#include "app_lucky_wheel/app_lucky_wheel.h"
#include "app_template/app_template.h"

namespace POCKET35_APPS
{
    struct AppIDs_t
    {
        int launcher = -1;
        int link = -1;
        int imu = -1;
        int battery = -1;
        int wifi = -1;
        int clock = -1;
        int lvglDemo = -1;
        int fluidbox = -1;
        int luckyWheel = -1;
        int appTemplate = -1;
    };

    inline AppIDs_t installApps(mooncake::Mooncake& mooncake)
    {
        AppIDs_t ids;

        ids.launcher = mooncake.installApp(std::make_unique<MOONCAKE::APPS::AppLauncher>());
        ids.link = mooncake.installApp(std::make_unique<MOONCAKE::APPS::AppLink>());
        ids.imu = mooncake.installApp(std::make_unique<MOONCAKE::APPS::AppImu>());
        ids.battery = mooncake.installApp(std::make_unique<MOONCAKE::APPS::AppBattery>());
        ids.wifi = mooncake.installApp(std::make_unique<MOONCAKE::APPS::AppWifi>());
        ids.clock = mooncake.installApp(std::make_unique<MOONCAKE::APPS::AppClock>());
        ids.lvglDemo = mooncake.installApp(std::make_unique<MOONCAKE::APPS::AppLvglDemo>());
        ids.fluidbox = mooncake.installApp(std::make_unique<MOONCAKE::APPS::AppFluidBox>());
        ids.luckyWheel = mooncake.installApp(std::make_unique<MOONCAKE::APPS::AppLuckyWheel>());
        ids.appTemplate = mooncake.installApp(std::make_unique<MOONCAKE::APPS::AppTemplate>());

        return ids;
    }
} // namespace POCKET35_APPS
