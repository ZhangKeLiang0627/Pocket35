#include "app_lvgl_demo.h"
#include "hal/hal.h"
#include "../assets/theme/theme.h"
#include <lvgl.h>
#include "lvgl/smooth_lvgl.hpp"
#include "uitk/short_namespace.hpp"

using namespace MOONCAKE::APPS;

namespace
{
    lv_obj_t* demoRoot = nullptr;
    AppLvglDemo* activeInstance = nullptr;

    void sliderChangedCb(lv_event_t* e)
    {
        lv_obj_t* slider = (lv_obj_t*)lv_event_get_target(e);
        lv_obj_t* valueLabel = (lv_obj_t*)lv_event_get_user_data(e);
        lv_label_set_text_fmt(valueLabel, "Value: %d", (int)lv_slider_get_value(slider));
    }

    void exitBtnCb(lv_event_t* e)
    {
        if (lv_event_get_code(e) == LV_EVENT_CLICKED && activeInstance != nullptr)
        {
            activeInstance->close();
        }
    }
} // namespace

void AppLvglDemo::onCreate()
{
    setAppInfo().name = "LVGL Demo";
    setAppInfo().icon = (void*)image_data_icon_app_lvgl;
    setAppInfo().userData = (void*)(uintptr_t)THEME::kAppImuTheme;
}

void AppLvglDemo::onOpen()
{
    activeInstance = this;
    HAL::LvglHideCanvas();

    demoRoot = lv_obj_create(lv_screen_active());
    lv_obj_set_size(demoRoot, 480, 320);
    lv_obj_set_pos(demoRoot, 0, 0);
    lv_obj_set_style_bg_color(demoRoot, lv_color_hex(0x101216), 0);
    lv_obj_set_style_border_width(demoRoot, 0, 0);
    lv_obj_set_style_pad_all(demoRoot, 0, 0);

    lv_obj_t* title = lv_label_create(demoRoot);
    lv_label_set_text(title, "LVGL Widget Demo");
    lv_obj_set_style_text_color(title, lv_color_hex(0xF2F4F8), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 24);

    lv_obj_t* slider = lv_slider_create(demoRoot);
    lv_obj_set_size(slider, 320, 12);
    lv_obj_align(slider, LV_ALIGN_CENTER, 0, -20);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, 50, LV_ANIM_OFF);

    lv_obj_t* valueLabel = lv_label_create(demoRoot);
    lv_label_set_text_fmt(valueLabel, "Value: %d", 50);
    lv_obj_set_style_text_color(valueLabel, lv_color_hex(0x8A8F98), 0);
    lv_obj_align(valueLabel, LV_ALIGN_CENTER, 0, 24);
    lv_obj_add_event_cb(slider, sliderChangedCb, LV_EVENT_VALUE_CHANGED, valueLabel);

    lv_obj_t* exitBtn = lv_button_create(demoRoot);
    lv_obj_set_size(exitBtn, 200, 48);
    lv_obj_align(exitBtn, LV_ALIGN_BOTTOM_MID, 0, -24);
    lv_obj_set_style_bg_color(exitBtn, lv_color_hex(0x8E5CF0), 0);
    lv_obj_set_style_radius(exitBtn, 24, 0);
    lv_obj_t* exitLabel = lv_label_create(exitBtn);
    lv_label_set_text(exitLabel, "EXIT");
    lv_obj_center(exitLabel);
    lv_obj_add_event_cb(exitBtn, exitBtnCb, LV_EVENT_CLICKED, nullptr);
}

void AppLvglDemo::onRunning() {}

void AppLvglDemo::onSleeping() {}

void AppLvglDemo::onClose()
{
    if (demoRoot != nullptr)
    {
        lv_obj_delete(demoRoot);
        demoRoot = nullptr;
    }
    activeInstance = nullptr;
    HAL::LvglShowCanvas();
}
