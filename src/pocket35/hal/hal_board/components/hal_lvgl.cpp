#include <Arduino.h>
#include <lvgl.h>
#include <esp_heap_caps.h>
#include "../hal_config.h"
#include "../hal_board.h"

namespace
{
    constexpr size_t kLvglDispDmaAlign = 64;

    lv_display_t* lvglDisplay = nullptr;
    lv_indev_t* lvglIndev = nullptr;
    uint16_t* lvglDispBuf = nullptr;
    bool lvglPaused = false;
} // namespace

static void lvglSwapPixelBytes(uint8_t* px_map, uint32_t count)
{
    if (((uintptr_t)px_map & 3) == 0)
    {
        uint32_t* quad = (uint32_t*)px_map;
        uint32_t pairs = count >> 1;
        for (uint32_t i = 0; i < pairs; i++)
        {
            uint32_t v = quad[i];
            quad[i] = ((v >> 8) & 0x00FF00FF) | ((v << 8) & 0xFF00FF00);
        }
        if (count & 1)
        {
            uint16_t* tail = (uint16_t*)px_map + (count - 1);
            *tail = (uint16_t)((*tail >> 8) | (*tail << 8));
        }
        return;
    }

    uint16_t* p = (uint16_t*)px_map;
    for (uint32_t i = 0; i < count; i++)
    {
        p[i] = (p[i] >> 8) | (p[i] << 8);
    }
}

static void lvglFlushCb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map)
{
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;

    lvglSwapPixelBytes(px_map, w * h);
    HAL::GetDisplay()->pushImage(area->x1, area->y1, w, h, (uint16_t*)px_map);
    lv_display_flush_ready(disp);
}

static void lvglTouchReadCb(lv_indev_t* indev, lv_indev_data_t* data)
{
    auto& touch = HAL::GetTouchPoint();
    data->point.x = touch.x;
    data->point.y = touch.y;
    data->state = touch.isPressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

void HAL_Board::lvglInit()
{
    lv_init();
    lv_tick_set_cb([]() { return (uint32_t)::millis(); });

    int32_t w = _canvas->width();
    int32_t h = _canvas->height();

    lvglDisplay = lv_display_create(w, h);
    lv_display_set_flush_cb(lvglDisplay, lvglFlushCb);

    size_t dispBufBytes = (size_t)w * (size_t)h * sizeof(uint16_t);
    lvglDispBuf = (uint16_t*)heap_caps_aligned_alloc(kLvglDispDmaAlign, dispBufBytes, MALLOC_CAP_SPIRAM);
    if (lvglDispBuf == nullptr)
    {
        popFatalError("lvgl disp buf alloc failed");
    }
    lv_display_set_buffers(lvglDisplay, lvglDispBuf, nullptr, dispBufBytes, LV_DISPLAY_RENDER_MODE_PARTIAL);

    lvglIndev = lv_indev_create();
    lv_indev_set_type(lvglIndev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(lvglIndev, lvglTouchReadCb);
}

void HAL_Board::lvglUpdate()
{
    if (!lvglPaused)
    {
        lv_timer_handler();
    }
}

void HAL_Board::lvglFlushNow()
{
    if (lvglDisplay != nullptr)
    {
        lv_refr_now(lvglDisplay);
    }
}

void HAL_Board::lvglSuspend()
{
    lvglPaused = true;
}

void HAL_Board::lvglResume()
{
    lvglPaused = false;
}

void HAL_Board::canvasUpdate()
{
    _canvas->pushSprite(0, 0);
}

void HAL_Board::canvasPush(int16_t x, int16_t y, int16_t w, int16_t h)
{
    const uint16_t* src = static_cast<const uint16_t*>(_canvas->getBuffer()) + static_cast<size_t>(y) * _canvas->width() + x;
    HAL::GetDisplay()->pushImage(x, y, w, h, src);
}
