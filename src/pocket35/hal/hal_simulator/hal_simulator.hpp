#pragma once
#ifndef ESP_PLATFORM
#define LGFX_USE_V1
#include <LGFX_AUTODETECT.hpp>
#include <LovyanGFX.hpp>
#include "../hal.h"

class HAL_Simulator : public HAL
{
public:
    std::string type() override { return "Simulator"; }

    void init() override;
    void updateTouch() override;
    void rtcUpdate() override;
    void pmicUpdate() override;

    void lvglInit() override;
    void lvglUpdate() override;
    void lvglFlushNow() override;
    void lvglSuspend() override;
    void lvglResume() override;

    void canvasUpdate() override;
    void canvasPush(int16_t x, int16_t y, int16_t w, int16_t h) override;

    void buttonUpdate() override;
    BUTTON::Event getButtonEvent() override;
    bool isButtonPressed() override;
    bool isButtonHolding() override;
    uint32_t getButtonPressedMs() override;
    void setButtonSfx(bool enabled) override;
};
#endif
