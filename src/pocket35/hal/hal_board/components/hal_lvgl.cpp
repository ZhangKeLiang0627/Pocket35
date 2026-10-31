#include <Arduino.h>
#include <lvgl.h>
#include <esp_heap_caps.h>
#include "../hal_config.h"
#include "../hal_board.h"

namespace
{
    lv_display_t* lvglDisplay = nullptr;
    lv_indev_t* lvglIndev = nullptr;
    lv_obj_t* lvglCanvas = nullptr;
    lv_color_t* lvglCanvasBuf = nullptr;
    lv_color_t lvglDispBuf[480 * 32];
} // namespace

static void lvglFlushCb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map)
{
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;
    uint16_t* p = (uint16_t*)px_map;
    uint32_t n = w * h;
    for (uint32_t i = 0; i < n; i++)
    {
        p[i] = (p[i] >> 8) | (p[i] << 8);
    }
    HAL::GetDisplay()->pushImage(area->x1, area->y1, w, h, p);
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
    lv_display_set_buffers(lvglDisplay, lvglDispBuf, nullptr, sizeof(lvglDispBuf) / sizeof(lvglDispBuf[0]),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    lvglCanvasBuf = (lv_color_t*)heap_caps_malloc(w * h * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    lvglCanvas = lv_canvas_create(lv_screen_active());
    lv_canvas_set_buffer(lvglCanvas, lvglCanvasBuf, w, h, LV_COLOR_FORMAT_RGB565);
    lv_obj_remove_flag(lvglCanvas, LV_OBJ_FLAG_SCROLLABLE);

    _canvas->setBuffer(lvglCanvasBuf, w, h);

    lvglIndev = lv_indev_create();
    lv_indev_set_type(lvglIndev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(lvglIndev, lvglTouchReadCb);
}

void HAL_Board::lvglUpdate()
{
    lv_timer_handler();
}

void HAL_Board::lvglHideCanvas()
{
    if (lvglCanvas != nullptr)
    {
        lv_obj_add_flag(lvglCanvas, LV_OBJ_FLAG_HIDDEN);
    }
}

void HAL_Board::lvglShowCanvas()
{
    if (lvglCanvas != nullptr)
    {
        lv_obj_remove_flag(lvglCanvas, LV_OBJ_FLAG_HIDDEN);
    }
}

void HAL_Board::canvasUpdate()
{
    if (lvglCanvas != nullptr)
    {
        lv_obj_invalidate(lvglCanvas);
        lv_refr_now(lvglDisplay);
    }
    else
    {
        _canvas->pushSprite(0, 0);
    }
}
