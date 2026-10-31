#include "app_lucky_wheel.h"
#include "hal/hal.h"
#include "apps/assets/icons/icons.h"
#include "apps/assets/theme/theme.h"
#include <lvgl.h>

using namespace MOONCAKE::APPS;

void AppLuckyWheel::onCreate()
{
    setAppInfo().name = "Lucky Wheel";
    setAppInfo().icon = (void*)image_data_icon_app_lucky_wheel;
    setAppInfo().userData = (void*)(uintptr_t)THEME::kAppLuckyWheelTheme;
}

void AppLuckyWheel::onOpen()
{
    HAL::LvglHideCanvas();

    selectionView = std::make_unique<view::SelectionView>();
    selectionView->init(lv_screen_active());
}

void AppLuckyWheel::onRunning()
{
    if (HAL::GetButtonEvent() == BUTTON::Event::click)
    {
        close();
        return;
    }

    if (selectionView)
    {
        if (selectionView->isConfirmed())
        {
            const int optionCount = selectionView->confirmedOptionCount();
            selectionView.reset();
            wheelView = std::make_unique<view::WheelView>();
            wheelView->init(lv_screen_active(), optionCount);
        }
        return;
    }

    if (wheelView)
    {
        wheelView->update();
    }
}

void AppLuckyWheel::onSleeping() {}

void AppLuckyWheel::onClose()
{
    wheelView.reset();
    selectionView.reset();
    HAL::LvglShowCanvas();
}
