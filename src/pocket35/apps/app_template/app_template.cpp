#include "app_template.h"
#include "hal/hal.h"
#include "apps/assets/theme/theme.h"

using namespace MOONCAKE::APPS;

namespace
{
    constexpr uint32_t kColorBackground = 0x101216;
    constexpr uint32_t kColorHeader = 0x1B1F27;
    constexpr uint32_t kColorText = 0xF2F4F8;
    constexpr uint32_t kColorMuted = 0x8A8F98;
    constexpr uint32_t kColorPanel = 0x181C23;

    constexpr int kHeaderHeight = 36;

    constexpr int kExitTop = 258;
    constexpr int kExitHeight = 44;
    constexpr int kExitWidth = 200;
    constexpr int kExitLeft = (480 - kExitWidth) / 2;
} // namespace

void AppTemplate::onCreate()
{
    setAppInfo().name = "Template";
    setAppInfo().icon = (void*)image_data_icon_app_template;
    setAppInfo().userData = (void*)(uintptr_t)THEME::kAppTemplateTheme;
}

void AppTemplate::onOpen()
{
    touchState = TouchState_t();
    render();
    HAL::CanvasUpdate();
}

void AppTemplate::onRunning()
{
    handleTouch();
}

void AppTemplate::onSleeping() {}

void AppTemplate::onClose() {}

bool AppTemplate::isInsideExit(int16_t x, int16_t y) const
{
    return x >= kExitLeft && x <= kExitLeft + kExitWidth && y >= kExitTop && y <= kExitTop + kExitHeight;
}

void AppTemplate::handleTouch()
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

void AppTemplate::render()
{
    auto* canvas = HAL::GetCanvas();

    canvas->fillScreen(kColorBackground);
    canvas->fillRect(0, 0, canvas->width(), kHeaderHeight, kColorHeader);

    canvas->setFont(&fonts::DejaVu18);
    canvas->setTextSize(1);
    canvas->setTextDatum(middle_left);
    canvas->setTextColor(kColorText, kColorHeader);
    canvas->drawString("TEMPLATE", 16, kHeaderHeight / 2);

    canvas->setFont(&fonts::DejaVu12);
    canvas->setTextDatum(top_left);
    canvas->setTextColor(kColorMuted, kColorBackground);
    canvas->drawString("This is a placeholder app.", 16, 96);

    const uint32_t exitColor = THEME::kAppTemplateTheme;
    canvas->fillSmoothRoundRect(kExitLeft, kExitTop, kExitWidth, kExitHeight, 12, exitColor);

    canvas->setFont(&fonts::DejaVu18);
    canvas->setTextDatum(middle_center);
    canvas->setTextColor(kColorText, exitColor);
    canvas->drawString("EXIT", kExitLeft + kExitWidth / 2, kExitTop + kExitHeight / 2);
}
