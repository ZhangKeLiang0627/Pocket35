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

    void init() override
    {
        _display = new LGFX(480, 320);
        _display->init();

        _canvas = new LGFX_SpriteFx(_display);
        _canvas->setColorDepth(16);
        _canvas->createSprite(_display->width(), _display->height());
    }
};
#endif
