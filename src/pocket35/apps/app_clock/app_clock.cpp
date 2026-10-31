#include "app_clock.h"
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

    constexpr int kHeaderHeight = 36;
    constexpr int kButtonTop = 258;
    constexpr int kButtonHeight = 44;
    constexpr int kButtonWidth = 216;
    constexpr int kButtonLeft0 = 16;
    constexpr int kButtonLeft1 = 248;

    const char* weekdayAbbr(int year, int month, int day)
    {
        static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
        static const char* names[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
        int y = year - (month < 3 ? 1 : 0);
        int wd = (y + y / 4 - y / 100 + y / 400 + t[month - 1] + day) % 7;
        return names[wd];
    }
} // namespace

void AppClock::onCreate()
{
    setAppInfo().name = "Clock";
    setAppInfo().icon = (void*)image_data_icon_app_clock;
    setAppInfo().userData = (void*)(uintptr_t)THEME::kAppClockTheme;
}

void AppClock::onOpen()
{
    touchState = TouchState_t();
    lastRenderMs = 0;
    syncing = false;
    HAL::RtcUpdate();
    render();
    HAL::CanvasUpdate();
}

void AppClock::onRunning()
{
    handleTouch();

    uint32_t now = HAL::Millis();
    if (now - lastRenderMs < 1000)
    {
        return;
    }
    lastRenderMs = now;

    HAL::RtcUpdate();
    render();
    HAL::CanvasUpdate();
}

void AppClock::onSleeping() {}

void AppClock::onClose() {}

bool AppClock::isInsideButton(int index, int16_t x, int16_t y) const
{
    int left = index == 0 ? kButtonLeft0 : kButtonLeft1;
    return x >= left && x <= left + kButtonWidth && y >= kButtonTop && y <= kButtonTop + kButtonHeight;
}

void AppClock::handleTouch()
{
    auto& touch = HAL::GetTouchPoint();

    if (touch.isPressed && !touchState.wasPressed)
    {
        touchState.pressedButton = -1;
        if (isInsideButton(0, touch.x, touch.y))
        {
            touchState.pressedButton = 0;
        }
        else if (isInsideButton(1, touch.x, touch.y))
        {
            touchState.pressedButton = 1;
        }
    }
    else if (!touch.isPressed && touchState.wasPressed)
    {
        if (touchState.pressedButton == 0 && isInsideButton(0, touch.x, touch.y))
        {
            syncing = true;
            render();
            HAL::CanvasUpdate();
            HAL::NtpSync();
            syncing = false;
            HAL::RtcUpdate();
        }
        else if (touchState.pressedButton == 1 && isInsideButton(1, touch.x, touch.y))
        {
            close();
        }
        touchState.pressedButton = -1;
    }

    touchState.wasPressed = touch.isPressed;
}

void AppClock::render()
{
    auto* canvas = HAL::GetCanvas();
    auto& rtc = HAL::GetRtcTime();
    auto& sync = HAL::GetRtcSyncInfo();

    canvas->fillScreen(kColorBackground);
    canvas->fillRect(0, 0, canvas->width(), kHeaderHeight, kColorHeader);

    char buffer[64];

    canvas->setFont(&fonts::DejaVu18);
    canvas->setTextSize(1);
    canvas->setTextDatum(middle_left);
    canvas->setTextColor(kColorText, kColorHeader);
    canvas->drawString("CLOCK", 16, kHeaderHeight / 2);

    canvas->setTextDatum(middle_right);
    canvas->setTextColor(kColorMuted, kColorHeader);
    canvas->drawString(sync.synced ? "NTP OK" : "NO NTP", canvas->width() - 16, kHeaderHeight / 2);

    canvas->fillSmoothRoundRect(16, 48, 448, 200, 14, kColorPanel);

    canvas->setFont(&fonts::DejaVu40);
    canvas->setTextDatum(middle_center);
    canvas->setTextColor(kColorText, kColorPanel);
    snprintf(buffer, sizeof(buffer), "%02d:%02d", rtc.hour, rtc.minute);
    canvas->drawString(buffer, 240, 106);

    canvas->setFont(&fonts::DejaVu18);
    canvas->setTextDatum(middle_center);
    snprintf(buffer, sizeof(buffer), "%s  %04d-%02d-%02d",
             weekdayAbbr(rtc.year, rtc.month, rtc.day), rtc.year, rtc.month, rtc.day);
    canvas->drawString(buffer, 240, 172);

    canvas->setFont(&fonts::DejaVu12);
    canvas->setTextColor(kColorMuted, kColorPanel);
    if (syncing)
    {
        snprintf(buffer, sizeof(buffer), "syncing...");
    }
    else if (sync.synced)
    {
        snprintf(buffer, sizeof(buffer), "last sync %04d-%02d-%02d %02d:%02d",
                 sync.year, sync.month, sync.day, sync.hour, sync.minute);
    }
    else
    {
        snprintf(buffer, sizeof(buffer), "not synced yet");
    }
    canvas->drawString(buffer, 240, 208);

    canvas->fillSmoothRoundRect(kButtonLeft0, kButtonTop, kButtonWidth, kButtonHeight, 12, THEME::kAppClockTheme);
    canvas->setFont(&fonts::DejaVu18);
    canvas->setTextDatum(middle_center);
    canvas->setTextColor(kColorText, THEME::kAppClockTheme);
    canvas->drawString("SYNC", kButtonLeft0 + kButtonWidth / 2, kButtonTop + kButtonHeight / 2);

    canvas->fillSmoothRoundRect(kButtonLeft1, kButtonTop, kButtonWidth, kButtonHeight, 12, kColorPanel);
    canvas->setTextColor(kColorText, kColorPanel);
    canvas->drawString("EXIT", kButtonLeft1 + kButtonWidth / 2, kButtonTop + kButtonHeight / 2);
}
