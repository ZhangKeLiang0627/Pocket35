#include "app_battery.h"
#include "hal/hal.h"
#include "apps/assets/fonts/fonts.h"
#include "apps/assets/theme/theme.h"
#include <cstdio>

using namespace MOONCAKE::APPS;

namespace
{
    constexpr uint32_t kColorBackground = 0x101216;
    constexpr uint32_t kColorHeader = 0x1B1F27;
    constexpr uint32_t kColorText = 0xF2F4F8;
    constexpr uint32_t kColorMuted = 0x8A8F98;
    constexpr uint32_t kColorPanel = 0x181C23;
    constexpr uint32_t kColorGood = 0x3DD68C;

    constexpr int kHeaderHeight = 36;

    constexpr int kExitTop = 258;
    constexpr int kExitHeight = 44;
    constexpr int kExitWidth = 200;
    constexpr int kExitLeft = (480 - kExitWidth) / 2;
} // namespace

void AppBattery::onCreate()
{
    setAppInfo().name = "Battery";
    setAppInfo().icon = (void*)image_data_icon_app_battery;
    setAppInfo().userData = (void*)(uintptr_t)THEME::kAppBatteryTheme;
}

void AppBattery::onOpen()
{
    touchState = TouchState_t();
    lastRenderMs = 0;
    HAL::PmicUpdate();
    render();
    HAL::CanvasUpdate();
}

void AppBattery::onRunning()
{
    handleTouch();

    uint32_t now = HAL::Millis();
    if (now - lastRenderMs < 1000)
    {
        return;
    }
    lastRenderMs = now;

    HAL::PmicUpdate();
    render();
    HAL::CanvasUpdate();
}

void AppBattery::onSleeping() {}

void AppBattery::onClose() {}

bool AppBattery::isInsideExit(int16_t x, int16_t y) const
{
    return x >= kExitLeft && x <= kExitLeft + kExitWidth && y >= kExitTop && y <= kExitTop + kExitHeight;
}

void AppBattery::handleTouch()
{
    auto& touch = HAL::GetTouchPoint();

    if (touch.isPressed && !touchState.wasPressed)
    {
        touchState.insideExit = isInsideExit(touch.x, touch.y);
    }
    else if (!touch.isPressed && touchState.wasPressed)
    {
        if (touchState.insideExit && isInsideExit(touch.x, touch.y))
        {
            close();
        }
        touchState.insideExit = false;
    }

    touchState.wasPressed = touch.isPressed;
}

void AppBattery::render()
{
    auto* canvas = HAL::GetCanvas();
    auto& battery = HAL::GetBattery();

    canvas->fillScreen(kColorBackground);
    canvas->fillRect(0, 0, canvas->width(), kHeaderHeight, kColorHeader);

    char buffer[32];

    canvas->setFont(&fonts::DejaVu18);
    canvas->setTextSize(1);
    canvas->setTextDatum(middle_left);
    canvas->setTextColor(kColorText, kColorHeader);
    canvas->drawString("BATTERY", 16, kHeaderHeight / 2);

    canvas->setTextDatum(middle_right);
    canvas->setTextColor(kColorMuted, kColorHeader);
    if (battery.valid)
    {
        snprintf(buffer, sizeof(buffer), "%.1f C", battery.dieTemp);
    }
    else
    {
        snprintf(buffer, sizeof(buffer), "--");
    }
    canvas->drawString(buffer, canvas->width() - 16, kHeaderHeight / 2);

    canvas->fillSmoothRoundRect(kExitLeft, kExitTop, kExitWidth, kExitHeight, 12, THEME::kAppBatteryTheme);
    canvas->setTextDatum(middle_center);
    canvas->setTextColor(kColorText, THEME::kAppBatteryTheme);
    canvas->drawString("EXIT", kExitLeft + kExitWidth / 2, kExitTop + kExitHeight / 2);

    if (!battery.valid)
    {
        canvas->setTextDatum(middle_center);
        canvas->setTextColor(kColorMuted, kColorBackground);
        canvas->drawString("PMIC not found", canvas->width() / 2, 150);
        return;
    }

    canvas->fillSmoothRoundRect(16, 48, 448, 100, 14, kColorPanel);

    const int bx = 40;
    const int by = 78;
    const int bw = 72;
    const int bh = 40;
    canvas->fillSmoothRoundRect(bx, by, bw, bh, 8, kColorMuted);
    canvas->fillSmoothRoundRect(bx + 3, by + 3, bw - 6, bh - 6, 5, kColorBackground);
    canvas->fillRect(bx + bw, by + bh / 2 - 8, 5, 16, kColorMuted);

    float pct = battery.percentage;
    if (pct < 0.0f)
    {
        pct = 0.0f;
    }
    if (pct > 100.0f)
    {
        pct = 100.0f;
    }
    int fillW = (int)((bw - 6) * pct / 100.0f);
    if (fillW > 0)
    {
        canvas->fillSmoothRoundRect(bx + 3, by + 3, fillW, bh - 6, 5, kColorGood);
    }

    canvas->setFont(&fonts::DejaVu40);
    canvas->setTextDatum(middle_left);
    canvas->setTextColor(kColorText, kColorPanel);
    snprintf(buffer, sizeof(buffer), "%.0f%%", pct);
    canvas->drawString(buffer, 140, 98);

    const char* stateText = "DISCHARGE";
    uint32_t stateColor = kColorText;
    if (!battery.present)
    {
        stateText = "NO BAT";
        stateColor = kColorMuted;
    }
    else if (battery.charging)
    {
        stateText = "CHARGING";
        stateColor = kColorGood;
    }
    else if (battery.chargeDone)
    {
        stateText = "FULL";
        stateColor = kColorGood;
    }

    canvas->setFont(&fonts::DejaVu18);
    canvas->setTextDatum(middle_right);
    canvas->setTextColor(stateColor, kColorPanel);
    canvas->drawString(stateText, 432, 98);

    canvas->fillSmoothRoundRect(16, 156, 448, 94, 14, kColorPanel);

    canvas->setFont(&fonts::DejaVu12);
    canvas->setTextDatum(middle_left);
    canvas->setTextColor(kColorMuted, kColorPanel);
    canvas->drawString("VOLTAGE", 32, 170);
    canvas->drawString("TEMP", 32, 216);
    canvas->drawString("CHARGE", 248, 170);
    canvas->drawString("POWER", 248, 216);

    canvas->setFont(&fonts::DejaVu18);
    canvas->setTextColor(kColorText, kColorPanel);
    snprintf(buffer, sizeof(buffer), "%.2f V", battery.voltage / 1000.0f);
    canvas->drawString(buffer, 32, 194);
    snprintf(buffer, sizeof(buffer), "%.1f C", battery.dieTemp);
    canvas->drawString(buffer, 32, 240);
    canvas->drawString(stateText, 248, 194);
    canvas->drawString(battery.vbus ? "USB" : "BATTERY", 248, 240);
}
