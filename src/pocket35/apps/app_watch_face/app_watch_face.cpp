#include "app_watch_face.h"
#include "hal/hal.h"
#include "apps/assets/icons/icons.h"
#include "apps/assets/theme/theme.h"
#include <lvgl.h>

using namespace MOONCAKE::APPS;

void AppWatchFace::onCreate()
{
    setAppInfo().name = "Watch Face";
    setAppInfo().icon = (void*)image_data_icon_app_watch_face;
    setAppInfo().userData = (void*)(uintptr_t)THEME::kAppWatchFaceTheme;
}

void AppWatchFace::onOpen()
{
    HAL::LvglHideCanvas();

    faceManager = std::make_unique<view::FaceManager>();
    faceManager->init(lv_screen_active());
}

void AppWatchFace::onRunning()
{
    if (HAL::GetButtonEvent() == BUTTON::Event::click)
    {
        close();
        return;
    }

    if (faceManager)
        faceManager->update();
}

void AppWatchFace::onSleeping() {}

void AppWatchFace::onClose()
{
    faceManager.reset();
    HAL::LvglShowCanvas();
}
