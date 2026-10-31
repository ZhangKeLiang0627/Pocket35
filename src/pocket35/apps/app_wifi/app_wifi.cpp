#include "app_wifi.h"
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
    constexpr uint32_t kColorWarn = 0xF0A63D;

    constexpr int kHeaderHeight = 36;

    constexpr int kButtonTop = 258;
    constexpr int kButtonHeight = 44;
    constexpr int kButtonWidth = 216;
    constexpr int kButtonLeft0 = 16;
    constexpr int kButtonLeft1 = 248;
} // namespace

void AppWifi::onCreate()
{
    setAppInfo().name = "WiFi";
    setAppInfo().icon = (void*)image_data_icon_app_wifi;
    setAppInfo().userData = (void*)(uintptr_t)THEME::kAppWifiTheme;
}

void AppWifi::onOpen()
{
    touchState = TouchState_t();
    lastRenderMs = 0;
    render();
    HAL::CanvasUpdate();
}

void AppWifi::onRunning()
{
    handleTouch();

    uint32_t now = HAL::Millis();
    if (now - lastRenderMs < 500)
    {
        return;
    }
    lastRenderMs = now;

    render();
    HAL::CanvasUpdate();
}

void AppWifi::onSleeping() {}

void AppWifi::onClose() {}

bool AppWifi::isInsideButton(int index, int16_t x, int16_t y) const
{
    int left = index == 0 ? kButtonLeft0 : kButtonLeft1;
    return x >= left && x <= left + kButtonWidth && y >= kButtonTop && y <= kButtonTop + kButtonHeight;
}

void AppWifi::handleTouch()
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
            HAL::WifiStartConfig();
        }
        else if (touchState.pressedButton == 1 && isInsideButton(1, touch.x, touch.y))
        {
            close();
        }
        touchState.pressedButton = -1;
    }

    touchState.wasPressed = touch.isPressed;
}

void AppWifi::render()
{
    auto* canvas = HAL::GetCanvas();
    auto& wifi = HAL::GetWifiInfo();

    canvas->fillScreen(kColorBackground);
    canvas->fillRect(0, 0, canvas->width(), kHeaderHeight, kColorHeader);

    char buffer[48];

    canvas->setFont(&fonts::DejaVu18);
    canvas->setTextSize(1);
    canvas->setTextDatum(middle_left);
    canvas->setTextColor(kColorText, kColorHeader);
    canvas->drawString("WIFI", 16, kHeaderHeight / 2);

    canvas->fillSmoothRoundRect(16, 48, 448, 200, 14, kColorPanel);

    const char* stateText = "IDLE";
    uint32_t stateColor = kColorMuted;
    if (wifi.state == WIFI::State::configAp)
    {
        stateText = "CONFIG MODE";
        stateColor = kColorWarn;
    }
    else if (wifi.state == WIFI::State::connecting)
    {
        stateText = "CONNECTING";
        stateColor = kColorWarn;
    }
    else if (wifi.state == WIFI::State::connected)
    {
        stateText = "CONNECTED";
        stateColor = kColorGood;
    }

    canvas->setFont(&fonts::DejaVu24);
    canvas->setTextDatum(middle_center);
    canvas->setTextColor(stateColor, kColorPanel);
    canvas->drawString(stateText, 240, 90);

    canvas->setFont(&fonts::DejaVu18);
    canvas->setTextDatum(middle_center);
    canvas->setTextColor(kColorText, kColorPanel);

    if (wifi.state == WIFI::State::configAp)
    {
        snprintf(buffer, sizeof(buffer), "AP: %s", wifi.ssid.c_str());
        canvas->drawString(buffer, 240, 138);
        snprintf(buffer, sizeof(buffer), "%s", wifi.ip.c_str());
        canvas->drawString(buffer, 240, 168);
        canvas->setFont(&fonts::DejaVu12);
        canvas->setTextColor(kColorMuted, kColorPanel);
        canvas->drawString("join this hotspot to open setup page", 240, 200);
    }
    else if (wifi.state == WIFI::State::connecting)
    {
        canvas->drawString(wifi.ssid.c_str(), 240, 138);
        canvas->setFont(&fonts::DejaVu12);
        canvas->setTextColor(kColorMuted, kColorPanel);
        canvas->drawString("waiting for IP...", 240, 168);
    }
    else if (wifi.state == WIFI::State::connected)
    {
        canvas->drawString(wifi.ssid.c_str(), 240, 132);
        snprintf(buffer, sizeof(buffer), "%s", wifi.ip.c_str());
        canvas->drawString(buffer, 240, 162);
        canvas->setFont(&fonts::DejaVu12);
        canvas->setTextColor(kColorMuted, kColorPanel);
        canvas->drawString("connected to wi-fi", 240, 194);
    }

    canvas->fillSmoothRoundRect(kButtonLeft0, kButtonTop, kButtonWidth, kButtonHeight, 12, THEME::kAppWifiTheme);
    canvas->setFont(&fonts::DejaVu18);
    canvas->setTextDatum(middle_center);
    canvas->setTextColor(kColorText, THEME::kAppWifiTheme);
    canvas->drawString("RECONFIG", kButtonLeft0 + kButtonWidth / 2, kButtonTop + kButtonHeight / 2);

    canvas->fillSmoothRoundRect(kButtonLeft1, kButtonTop, kButtonWidth, kButtonHeight, 12, kColorPanel);
    canvas->setTextColor(kColorText, kColorPanel);
    canvas->drawString("EXIT", kButtonLeft1 + kButtonWidth / 2, kButtonTop + kButtonHeight / 2);
}
