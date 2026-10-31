#include "app_imu.h"
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

    constexpr int kExitTop = 258;
    constexpr int kExitHeight = 44;
    constexpr int kExitWidth = 200;
    constexpr int kExitLeft = (480 - kExitWidth) / 2;
} // namespace

void AppImu::onCreate()
{
    setAppInfo().name = "Motion";
    setAppInfo().icon = (void*)image_data_icon_app_imu;
    setAppInfo().userData = (void*)(uintptr_t)THEME::kAppImuTheme;
}

void AppImu::onOpen()
{
    touchState = TouchState_t();
    lastRenderMs = 0;
    render();
    HAL::CanvasUpdate();
}

void AppImu::onRunning()
{
    handleTouch();

    uint32_t now = HAL::Millis();
    if (now - lastRenderMs < 50)
    {
        return;
    }
    lastRenderMs = now;

    HAL::ImuUpdate();
    render();
    HAL::CanvasUpdate();
}

void AppImu::onSleeping() {}

void AppImu::onClose() {}

bool AppImu::isInsideExit(int16_t x, int16_t y) const
{
    return x >= kExitLeft && x <= kExitLeft + kExitWidth && y >= kExitTop && y <= kExitTop + kExitHeight;
}

void AppImu::handleTouch()
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

void AppImu::render()
{
    auto* canvas = HAL::GetCanvas();
    auto& imu = HAL::GetImuData();

    canvas->fillScreen(kColorBackground);
    canvas->fillRect(0, 0, canvas->width(), kHeaderHeight, kColorHeader);

    char buffer[32];

    canvas->setFont(&fonts::DejaVu18);
    canvas->setTextSize(1);
    canvas->setTextDatum(middle_left);
    canvas->setTextColor(kColorText, kColorHeader);
    canvas->drawString("MOTION", 16, kHeaderHeight / 2);

    if (imu.present)
    {
        snprintf(buffer, sizeof(buffer), "%.1f C", imu.temperature);
    }
    else
    {
        snprintf(buffer, sizeof(buffer), "--");
    }
    canvas->setTextDatum(middle_right);
    canvas->setTextColor(kColorMuted, kColorHeader);
    canvas->drawString(buffer, canvas->width() - 16, kHeaderHeight / 2);

    canvas->fillSmoothRoundRect(kExitLeft, kExitTop, kExitWidth, kExitHeight, 12, THEME::kAppImuTheme);
    canvas->setTextDatum(middle_center);
    canvas->setTextColor(kColorText, THEME::kAppImuTheme);
    canvas->drawString("EXIT", kExitLeft + kExitWidth / 2, kExitTop + kExitHeight / 2);

    if (!imu.present)
    {
        canvas->setTextDatum(middle_center);
        canvas->setTextColor(kColorMuted, kColorBackground);
        canvas->drawString("IMU not found", canvas->width() / 2, 150);
        return;
    }

    canvas->fillSmoothRoundRect(16, 48, 448, 64, 14, kColorPanel);
    canvas->setFont(&fonts::DejaVu12);
    canvas->setTextDatum(top_left);
    canvas->setTextColor(kColorMuted, kColorPanel);
    canvas->drawString("ROLL", 32, 58);
    canvas->drawString("PITCH", 240, 58);
    canvas->setFont(&fonts::DejaVu18);
    canvas->setTextColor(kColorText, kColorPanel);
    snprintf(buffer, sizeof(buffer), "%+.1f deg", imu.roll);
    canvas->drawString(buffer, 32, 80);
    snprintf(buffer, sizeof(buffer), "%+.1f deg", imu.pitch);
    canvas->drawString(buffer, 240, 80);

    canvas->fillSmoothRoundRect(16, 120, 216, 130, 14, kColorPanel);
    canvas->setFont(&fonts::DejaVu12);
    canvas->setTextColor(kColorMuted, kColorPanel);
    canvas->drawString("ACCEL (m/s2)", 32, 132);
    canvas->setFont(&fonts::DejaVu18);
    canvas->setTextColor(kColorText, kColorPanel);
    snprintf(buffer, sizeof(buffer), "X %+.2f", imu.accelX);
    canvas->drawString(buffer, 32, 156);
    snprintf(buffer, sizeof(buffer), "Y %+.2f", imu.accelY);
    canvas->drawString(buffer, 32, 184);
    snprintf(buffer, sizeof(buffer), "Z %+.2f", imu.accelZ);
    canvas->drawString(buffer, 32, 212);

    canvas->fillSmoothRoundRect(248, 120, 216, 130, 14, kColorPanel);
    canvas->setFont(&fonts::DejaVu12);
    canvas->setTextColor(kColorMuted, kColorPanel);
    canvas->drawString("GYRO (deg/s)", 264, 132);
    canvas->setFont(&fonts::DejaVu18);
    canvas->setTextColor(kColorText, kColorPanel);
    snprintf(buffer, sizeof(buffer), "X %+.2f", imu.gyroX);
    canvas->drawString(buffer, 264, 156);
    snprintf(buffer, sizeof(buffer), "Y %+.2f", imu.gyroY);
    canvas->drawString(buffer, 264, 184);
    snprintf(buffer, sizeof(buffer), "Z %+.2f", imu.gyroZ);
    canvas->drawString(buffer, 264, 212);
}
