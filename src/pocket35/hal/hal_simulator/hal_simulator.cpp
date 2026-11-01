#ifndef ESP_PLATFORM

#include "hal_simulator.hpp"
#include "../utils/lvgl_pixel_swap.h"
#include <SDL.h>
#include <cstring>
#include <ctime>
#include <lvgl.h>

namespace
{
    constexpr int32_t kPanelWidth = 480;
    constexpr int32_t kPanelHeight = 320;

    lv_display_t* lvglDisplay = nullptr;
    lv_indev_t* lvglIndev = nullptr;
    uint16_t* lvglDispBuf = nullptr;
    bool lvglPaused = false;
    BUTTON::Event pendingButtonEvent = BUTTON::Event::none;

    bool bootKeyDown()
    {
        const Uint8* keys = SDL_GetKeyboardState(nullptr);
        return keys != nullptr && keys[SDL_SCANCODE_SPACE] != 0;
    }

    void lvglFlushCb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map)
    {
        const int32_t w = area->x2 - area->x1 + 1;
        const int32_t h = area->y2 - area->y1 + 1;
        PIXEL::swapRgb565Bytes(px_map, static_cast<uint32_t>(w * h));
        HAL::GetDisplay()->pushImage(area->x1, area->y1, w, h, reinterpret_cast<uint16_t*>(px_map));
        lv_display_flush_ready(disp);
    }

    void lvglTouchReadCb(lv_indev_t*, lv_indev_data_t* data)
    {
        auto& touch = HAL::GetTouchPoint();
        data->point.x = touch.x;
        data->point.y = touch.y;
        data->state = touch.isPressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    }

    uint32_t lvglTickMs() { return HAL::Millis(); }
} // namespace

void HAL_Simulator::init()
{
    _display = new LGFX(kPanelWidth, kPanelHeight);
    _display->init();

    _canvas = new LGFX_SpriteFx(_display);
    _canvas->setColorDepth(16);
    _canvas->createSprite(_display->width(), _display->height());

    lvglInit();

    btnBoot.setRawState(millis(), false);

    _touch.isPressed = false;
    printf("[sim] panel %dx%d, space key -> BOOT\n", _display->width(), _display->height());
}

void HAL_Simulator::updateTouch()
{
    lgfx::touch_point_t tp;
    const uint_fast8_t count = _display->getTouch(&tp);
    if (count > 0)
    {
        _touch.x = static_cast<uint16_t>(tp.x);
        _touch.y = static_cast<uint16_t>(tp.y);
        _touch.isPressed = true;
    }
    else
    {
        _touch.isPressed = false;
    }
}

void HAL_Simulator::rtcUpdate()
{
    time_t now = 0;
    time(&now);
    tm local = *localtime(&now);

    _rtc_time.year = local.tm_year + 1900;
    _rtc_time.month = local.tm_mon + 1;
    _rtc_time.day = local.tm_mday;
    _rtc_time.hour = local.tm_hour;
    _rtc_time.minute = local.tm_min;
    _rtc_time.second = local.tm_sec;
    _rtc_time.valid = true;
}

void HAL_Simulator::pmicUpdate()
{
    _battery.present = true;
    _battery.charging = true;
    _battery.chargeDone = false;
    _battery.vbus = true;
    _battery.voltage = 4.10f;
    _battery.percentage = 82.0f;
    _battery.dieTemp = 32.0f;
    _battery.valid = true;
}

void HAL_Simulator::lvglInit()
{
    lv_init();
    lv_tick_set_cb(lvglTickMs);

    const int32_t w = _canvas->width();
    const int32_t h = _canvas->height();

    lvglDisplay = lv_display_create(w, h);
    lv_display_set_flush_cb(lvglDisplay, lvglFlushCb);

    const size_t dispBufBytes = static_cast<size_t>(w) * static_cast<size_t>(h) * sizeof(uint16_t);
    lvglDispBuf = static_cast<uint16_t*>(malloc(dispBufBytes));
    if (lvglDispBuf == nullptr)
    {
        printf("[sim] lvgl disp buf alloc failed\n");
        return;
    }
    lv_display_set_buffers(lvglDisplay, lvglDispBuf, nullptr, dispBufBytes, LV_DISPLAY_RENDER_MODE_PARTIAL);

    lvglIndev = lv_indev_create();
    lv_indev_set_type(lvglIndev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(lvglIndev, lvglTouchReadCb);

    printf("[sim] lvgl ready %dx%d\n", w, h);
}

void HAL_Simulator::lvglUpdate()
{
    if (!lvglPaused && lvglDisplay != nullptr)
    {
        lv_timer_handler();
    }
}

void HAL_Simulator::lvglFlushNow()
{
    if (lvglDisplay != nullptr)
    {
        lv_refr_now(lvglDisplay);
    }
}

void HAL_Simulator::lvglSuspend() { lvglPaused = true; }

void HAL_Simulator::lvglResume() { lvglPaused = false; }

void HAL_Simulator::canvasUpdate()
{
    _canvas->pushSprite(0, 0);
    _display->display();
}

void HAL_Simulator::canvasPush(int16_t x, int16_t y, int16_t w, int16_t h)
{
    const uint16_t* src = static_cast<const uint16_t*>(_canvas->getBuffer()) +
                          static_cast<size_t>(y) * _canvas->width() + x;
    _display->pushImage(x, y, w, h, src);
}

void HAL_Simulator::buttonUpdate()
{
    btnBoot.setRawState(millis(), bootKeyDown());

    if (btnBoot.wasHold())
    {
        pendingButtonEvent = BUTTON::Event::hold;
    }
    else if (btnBoot.wasReleasedAfterHold())
    {
        pendingButtonEvent = BUTTON::Event::holdRelease;
    }
    else if (btnBoot.wasClicked())
    {
        pendingButtonEvent = BUTTON::Event::click;
    }
}

BUTTON::Event HAL_Simulator::getButtonEvent()
{
    const BUTTON::Event event = pendingButtonEvent;
    pendingButtonEvent = BUTTON::Event::none;
    return event;
}

bool HAL_Simulator::isButtonPressed() { return btnBoot.isPressed(); }

bool HAL_Simulator::isButtonHolding() { return btnBoot.isHolding(); }

uint32_t HAL_Simulator::getButtonPressedMs()
{
    return btnBoot.isPressed() ? millis() - btnBoot.lastChange() : 0;
}

void HAL_Simulator::setButtonSfx(bool enabled) { buttonConfig.sfxEnabled = enabled; }

#endif
