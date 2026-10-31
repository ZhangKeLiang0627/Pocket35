#include "app_link.h"
#include <cstdio>
#include "hal/hal.h"
#include "kt6368a_profile.h"
#include "apps/assets/icons/icons.h"
#include "apps/assets/theme/theme.h"

using namespace MOONCAKE::APPS;

namespace
{
    constexpr uint32_t kColorBackground = 0x101216;
    constexpr uint32_t kColorHeader = 0x1B1F27;
    constexpr uint32_t kColorAccent = 0x3D7EFF;
    constexpr uint32_t kColorText = 0xF2F4F8;
    constexpr uint32_t kColorMuted = 0x8A8F98;
    constexpr uint32_t kColorPanel = 0x181C23;
    constexpr uint32_t kColorOk = 0x35C759;
    constexpr uint32_t kColorWarn = 0xF5A623;

    constexpr int kHeaderHeight = 36;
    constexpr int kButtonTop = 262;
    constexpr int kButtonHeight = 42;
    constexpr int kButtonWidth = 130;
    constexpr int kButtonGap = 14;
    constexpr int kButtonLeft = 16;

    struct Button_t
    {
        const char* label;
        bool isAccent;
    };

    const Button_t kButtons[3] = {
        {"SCAN", true},
        {"PING", false},
        {"EXIT", false},
    };

    constexpr int kButtonCount = 3;
} // namespace

const char* AppLink::stateText(BLE::State state)
{
    switch (state)
    {
        case BLE::State::scanning:
            return "SCANNING";
        case BLE::State::connecting:
            return "CONNECTING";
        case BLE::State::connected:
            return "CONNECTED";
        default:
            return "IDLE";
    }
}

void AppLink::onCreate()
{
    setAppInfo().name = "Link";
    setAppInfo().icon = (void*)image_data_icon_app_link;
    setAppInfo().userData = (void*)(uintptr_t)THEME::kAppLinkTheme;
}

void AppLink::onOpen()
{
    bridge.begin();
    HAL::BleInit();
    HAL::BleOnReceive([this](const uint8_t* data, size_t length) { bridge.onReceived(data, length); });
    HAL::BleStartScan();

    touchState = TouchState_t();
    lastRenderMs = 0;
}

void AppLink::onRunning()
{
    HAL::BleUpdate();
    bridge.poll();
    handleTouch();

    if (HAL::Millis() - lastRenderMs >= 33)
    {
        render();
        HAL::CanvasUpdate();
        lastRenderMs = HAL::Millis();
    }
}

void AppLink::onSleeping() {}

void AppLink::onClose()
{
    HAL::BleDisconnect();
    bridge.setEchoEnabled(false);
}

bool AppLink::isInside(int index, int16_t x, int16_t y) const
{
    const int left = kButtonLeft + index * (kButtonWidth + kButtonGap);
    return x >= left && x <= left + kButtonWidth && y >= kButtonTop && y <= kButtonTop + kButtonHeight;
}

void AppLink::handleTouch()
{
    auto& touch = HAL::GetTouchPoint();

    if (touch.isPressed && !touchState.wasPressed)
    {
        touchState.pressedButton = -1;
        for (int i = 0; i < kButtonCount; i++)
        {
            if (isInside(i, touch.x, touch.y))
            {
                touchState.pressedButton = i;
                break;
            }
        }
    }
    else if (!touch.isPressed && touchState.wasPressed)
    {
        if (touchState.pressedButton >= 0 && isInside(touchState.pressedButton, touch.x, touch.y))
        {
            switch (touchState.pressedButton)
            {
                case 0:
                    HAL::BleStartScan();
                    break;
                case 1:
                    bridge.sendText("ping\r\n");
                    break;
                default:
                    close();
                    break;
            }
        }
        touchState.pressedButton = -1;
    }

    touchState.wasPressed = touch.isPressed;
}

void AppLink::render()
{
    renderHeader();
    renderStatus();
    renderStats();
    renderPayload();
    renderButtons();
}

void AppLink::renderHeader()
{
    auto* canvas = HAL::GetCanvas();
    canvas->fillScreen(kColorBackground);
    canvas->fillRect(0, 0, canvas->width(), kHeaderHeight, kColorHeader);

    canvas->setFont(&fonts::DejaVu18);
    canvas->setTextSize(1);
    canvas->setTextDatum(middle_left);
    canvas->setTextColor(kColorText, kColorHeader);
    canvas->drawString("LINK", 16, kHeaderHeight / 2);

    const auto state = HAL::GetBleState();
    const uint32_t stateColor = state == BLE::State::connected ? kColorOk : kColorWarn;
    canvas->setTextDatum(middle_right);
    canvas->setTextColor(stateColor, kColorHeader);
    canvas->drawString(stateText(state), canvas->width() - 16, kHeaderHeight / 2);
}

void AppLink::renderStatus()
{
    auto* canvas = HAL::GetCanvas();
    auto& peer = HAL::GetBlePeer();
    auto& stats = HAL::GetBleStats();

    canvas->setTextSize(1);

    canvas->setFont(&fonts::DejaVu12);
    canvas->setTextDatum(top_left);
    canvas->setTextColor(kColorMuted, kColorBackground);
    canvas->drawString("PEER", 16, 46);

    canvas->setFont(&fonts::DejaVu18);
    canvas->setTextColor(kColorText, kColorBackground);
    canvas->drawString(peer.name.empty() ? "-" : peer.name.c_str(), 16, 60);

    canvas->setFont(&fonts::DejaVu12);
    canvas->setTextColor(kColorMuted, kColorBackground);
    canvas->drawString("ADDR", 16, 84);

    canvas->setFont(&fonts::DejaVu18);
    canvas->setTextColor(kColorText, kColorBackground);
    canvas->drawString(peer.address.empty() ? "-" : peer.address.c_str(), 16, 98);

    char buffer[96];
    snprintf(buffer, sizeof(buffer), "RSSI %d dBm   MTU %u", peer.rssi, stats.mtu);
    canvas->setFont(&fonts::DejaVu12);
    canvas->setTextColor(kColorMuted, kColorBackground);
    canvas->drawString(buffer, 16, 124);
}

void AppLink::renderStats()
{
    auto* canvas = HAL::GetCanvas();
    auto& stats = HAL::GetBleStats();

    canvas->fillSmoothRoundRect(16, 148, 200, 96, 14, kColorPanel);

    canvas->setTextSize(1);

    canvas->setFont(&fonts::DejaVu12);
    canvas->setTextDatum(top_left);
    canvas->setTextColor(kColorMuted, kColorPanel);
    canvas->drawString("TX", 32, 158);

    char buffer[32];
    canvas->setFont(&fonts::DejaVu18);
    canvas->setTextColor(kColorText, kColorPanel);
    snprintf(buffer, sizeof(buffer), "%lu", static_cast<unsigned long>(stats.txPackets));
    canvas->drawString(buffer, 32, 174);

    canvas->setFont(&fonts::DejaVu12);
    canvas->setTextColor(kColorMuted, kColorPanel);
    snprintf(buffer, sizeof(buffer), "%lu bytes", static_cast<unsigned long>(stats.txBytes));
    canvas->drawString(buffer, 32, 206);

    canvas->setFont(&fonts::DejaVu12);
    canvas->setTextColor(kColorMuted, kColorPanel);
    canvas->drawString("RX", 120, 158);

    canvas->setFont(&fonts::DejaVu18);
    canvas->setTextColor(kColorText, kColorPanel);
    snprintf(buffer, sizeof(buffer), "%lu", static_cast<unsigned long>(stats.rxPackets));
    canvas->drawString(buffer, 120, 174);

    canvas->setFont(&fonts::DejaVu12);
    canvas->setTextColor(kColorMuted, kColorPanel);
    snprintf(buffer, sizeof(buffer), "%lu bytes", static_cast<unsigned long>(stats.rxBytes));
    canvas->drawString(buffer, 120, 206);
}

void AppLink::renderPayload()
{
    auto* canvas = HAL::GetCanvas();

    canvas->fillSmoothRoundRect(232, 148, 232, 96, 14, kColorPanel);

    canvas->setTextSize(1);
    canvas->setFont(&fonts::DejaVu12);
    canvas->setTextDatum(top_left);
    canvas->setTextColor(kColorMuted, kColorPanel);
    canvas->drawString("LAST RX", 248, 158);

    const std::string hex = bridge.lastHex();
    canvas->setFont(&fonts::DejaVu18);
    canvas->setTextColor(kColorText, kColorPanel);
    canvas->drawString(hex.empty() ? "-" : hex.substr(0, 24).c_str(), 248, 176);
}

void AppLink::renderButtons()
{
    auto* canvas = HAL::GetCanvas();

    canvas->setFont(&fonts::DejaVu18);
    canvas->setTextSize(1);
    canvas->setTextDatum(middle_center);

    for (int i = 0; i < kButtonCount; i++)
    {
        const int left = kButtonLeft + i * (kButtonWidth + kButtonGap);
        const uint32_t background = kButtons[i].isAccent ? kColorAccent : kColorPanel;
        canvas->fillSmoothRoundRect(left, kButtonTop, kButtonWidth, kButtonHeight, 12, background);
        canvas->setTextColor(kColorText, background);
        canvas->drawString(kButtons[i].label, left + kButtonWidth / 2, kButtonTop + kButtonHeight / 2);
    }
}
