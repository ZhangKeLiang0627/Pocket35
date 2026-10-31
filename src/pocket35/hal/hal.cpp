#include "hal.h"

HAL* HAL::_hal = nullptr;

HAL* HAL::Get() { return _hal; }

bool HAL::Check() { return _hal != nullptr; }

bool HAL::Inject(HAL* hal)
{
    if (_hal != nullptr || hal == nullptr)
    {
        return false;
    }

    _hal = hal;
    _hal->init();
    return true;
}

void HAL::Destroy()
{
    if (_hal == nullptr)
    {
        return;
    }

    delete _hal;
    _hal = nullptr;
}

void HAL::delay(unsigned long milliseconds) { lgfx::delay(milliseconds); }

unsigned long HAL::millis() { return lgfx::millis(); }

tm* HAL::getLocalTime()
{
    time(&_time_buffer);
    return localtime(&_time_buffer);
}

void HAL::canvasUpdate() { _canvas->pushSprite(0, 0); }

void HAL::renderFpsPanel()
{
    static unsigned long lastUpdateMs = 0;

    _canvas->setTextColor(TFT_WHITE, TFT_BLACK);
    _canvas->setTextSize(2);
    _canvas->drawNumber(1000 / (millis() - lastUpdateMs), 0, 0, &fonts::Font0);

    lastUpdateMs = millis();
}

void HAL::popFatalError(std::string msg)
{
    static const uint32_t background = 0x0078D7;

    _canvas->fillScreen(background);
    _canvas->setTextColor(TFT_WHITE, background);
    _canvas->setTextSize(5);
    _canvas->setCursor(8, 15);
    _canvas->printf(":(");

    _canvas->setTextSize(1);
    _canvas->setCursor(0, 155);
    _canvas->printf("   Fatal Error");
    _canvas->setCursor(0, 185);
    _canvas->printf("   %s", msg.c_str());

    _canvas->pushSprite(0, 0);

    while (1)
    {
        delay(1000);
    }
}

void HAL::loadLauncherFont24()
{
    _canvas->setFont(&fonts::DejaVu18);
    _canvas->setTextSize(1);
}
