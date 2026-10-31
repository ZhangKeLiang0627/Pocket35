#include "hal.hpp"

#include <Arduino.h>

using namespace smooth_ui_toolkit;

namespace
{
    std::function<uint32_t(void)> customGetTick;
    std::function<void(uint32_t)> customDelay;
} // namespace

void ui_hal::on_get_tick(std::function<uint32_t(void)> onGetTick)
{
    customGetTick = onGetTick;
}

uint32_t ui_hal::get_tick()
{
    if (customGetTick)
    {
        return customGetTick();
    }
    return static_cast<uint32_t>(::millis());
}

float ui_hal::get_tick_s()
{
    const uint32_t ms = get_tick();
    if (ms == 0)
    {
        return 0.0f;
    }
    return static_cast<float>(ms) / 1000.0f;
}

void ui_hal::on_delay(std::function<void(uint32_t)> onDelay)
{
    customDelay = onDelay;
}

void ui_hal::delay(uint32_t ms)
{
    if (customDelay)
    {
        customDelay(ms);
        return;
    }
    ::delay(ms);
}

void ui_hal::delay_s(float s)
{
    delay(static_cast<uint32_t>(s * 1000.0f));
}
