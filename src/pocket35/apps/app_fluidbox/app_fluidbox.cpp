#include "app_fluidbox.h"
#include "hal/hal.h"
#include "apps/assets/icons/icons.h"
#include "apps/assets/theme/theme.h"
#include <esp_timer.h>
#include <new>

using namespace MOONCAKE::APPS;

namespace
{
    constexpr int64_t periodMicros(uint16_t rate) { return 1000000LL / rate; }
} // namespace

void AppFluidBox::onCreate()
{
    setAppInfo().name = "FluidBox";
    setAppInfo().icon = (void*)image_data_icon_app_imu;
    setAppInfo().userData = (void*)(uintptr_t)THEME::kAppFluidboxTheme;
}

void AppFluidBox::onOpen()
{
    HAL::LvglHideCanvas();

    fluid.reset(new (std::nothrow) FluidBox());
    auto* canvas = HAL::GetCanvas();
    fluidReady = fluid && fluid->begin(canvas->width(), canvas->height());

    const int64_t now = esp_timer_get_time();
    nextImuMicros = now;
    nextSimMicros = now;
    nextRenderMicros = now;
    lastImuMicros = 0;
    touchDown = false;

    canvas->fillScreen(TFT_BLACK);
    canvas->pushSprite(0, 0);
}

void AppFluidBox::onRunning()
{
    if (HAL::GetButtonEvent() == BUTTON::Event::click)
    {
        close();
        return;
    }

    if (!fluidReady)
    {
        return;
    }

    updateTouch();

    int64_t now = esp_timer_get_time();
    if (now >= nextImuMicros)
    {
        nextImuMicros = now + periodMicros(fluidbox_config::kImuHz);
        updateImu(now);
    }

    now = esp_timer_get_time();
    if (now >= nextSimMicros)
    {
        nextSimMicros = now + periodMicros(fluidbox_config::kSimulationHz);
        fluid->step(1.0f / static_cast<float>(fluidbox_config::kSimulationHz));
    }

    now = esp_timer_get_time();
    if (now >= nextRenderMicros)
    {
        nextRenderMicros = now + periodMicros(fluidbox_config::kRenderHz);
        renderFrame();
    }
}

void AppFluidBox::onSleeping() {}

void AppFluidBox::onClose()
{
    fluidReady = false;
    touchDown = false;
    fluid.reset();
    HAL::LvglShowCanvas();
}

void AppFluidBox::renderFrame()
{
    auto* canvas = HAL::GetCanvas();
    if (fluidReady)
    {
        fluid->render(*canvas);
    }
    else
    {
        canvas->fillScreen(TFT_BLACK);
    }
    canvas->pushSprite(0, 0);
}

void AppFluidBox::updateImu(int64_t nowMicros)
{
    HAL::ImuUpdate();
    const auto& imu = HAL::GetImuData();

    float dt = 1.0f / static_cast<float>(fluidbox_config::kImuHz);
    if (lastImuMicros != 0 && nowMicros > lastImuMicros)
    {
        dt = static_cast<float>(nowMicros - lastImuMicros) * 1.0e-6f;
    }
    lastImuMicros = nowMicros;

    fluid->setImuSample(imu.accelX, imu.accelY, imu.accelZ, imu.gyroX, imu.gyroY, imu.gyroZ, dt);
}

void AppFluidBox::updateTouch()
{
    const auto& touch = HAL::GetTouchPoint();
    const bool down = touch.isPressed;

    if (down && !touchDown && fluidReady)
    {
        fluid->applyTouchImpulse(static_cast<float>(touch.x), static_cast<float>(touch.y));
    }
    touchDown = down;
}
