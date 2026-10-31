#include "watch_face.h"
#include "../assets/assets.h"
#include "hal/hal.h"
#include <cmath>
#include <cstdio>
#include <esp_heap_caps.h>

using namespace view;
using namespace uitk;
using namespace uitk::lvgl_cpp;

namespace
{
    constexpr int kCenterX = 240;
    constexpr int kCenterY = 160;
    constexpr int kDialSize = 300;
    constexpr int kDialRadius = kDialSize / 2;
    constexpr int kTickGap = 4;
    constexpr int kHourTickLength = 19;
    constexpr int kMinuteTickLength = 10;
    constexpr int kTickWidth = 2;
    constexpr int kTimeYOffset = 78;
    constexpr int kWeekdayYOffset = 108;
    constexpr int kDatePanelWidth = 44;
    constexpr int kDatePanelHeight = 32;
    constexpr int kDatePanelXOffset = 88;

    constexpr int kHourAnchor = 8;
    constexpr int kMinuteAnchor = 13;
    constexpr int kSecondAnchor = 20;

    constexpr uint32_t kDialColor = 0x1B1B1B;
    constexpr uint32_t kDatePanelColor = 0x101010;
    constexpr uint32_t kHourTickColor = 0x6E6E6E;
    constexpr uint32_t kMinuteTickColor = 0x454545;
    constexpr uint32_t kTimeColor = 0xD9D9D9;
    constexpr uint32_t kWeekdayColor = 0x717171;
    constexpr uint32_t kDateColor = 0x8B8B8B;
    constexpr int kDisplayModeCount = 3;

    constexpr const char* kWeekdayNames[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

    uint16_t* tickBuffer = nullptr;

    uint16_t* acquireTickBuffer()
    {
        if (tickBuffer != nullptr)
            return tickBuffer;

        tickBuffer = static_cast<uint16_t*>(
            heap_caps_aligned_alloc(64, kDialSize * kDialSize * sizeof(uint16_t), MALLOC_CAP_SPIRAM));
        return tickBuffer;
    }

    void drawTicks(Canvas& canvas)
    {
        uint16_t* buffer = acquireTickBuffer();
        if (buffer != nullptr)
            lv_canvas_set_buffer(canvas.get(), buffer, kDialSize, kDialSize, LV_COLOR_FORMAT_RGB565);
        else
            canvas.createBuffer(kDialSize, kDialSize, LV_COLOR_FORMAT_RGB565);

        constexpr float kPi = 3.14159265358979323846f;
        const int outerRadius = kDialRadius - kTickGap;

        canvas.fillBg(lv_color_black(), LV_OPA_COVER);
        canvas.startDrawing();

        for (int index = 0; index < 60; index++)
        {
            const bool isHourTick = index % 5 == 0;
            const int tickLength = isHourTick ? kHourTickLength : kMinuteTickLength;
            const lv_color_t color = isHourTick ? lv_color_hex(kHourTickColor) : lv_color_hex(kMinuteTickColor);

            const float angle = (-90.0f + index * 6.0f) * kPi / 180.0f;
            const int innerRadius = outerRadius - tickLength;

            const int x1 = static_cast<int>(lroundf(kDialRadius + innerRadius * cosf(angle)));
            const int y1 = static_cast<int>(lroundf(kDialRadius + innerRadius * sinf(angle)));
            const int x2 = static_cast<int>(lroundf(kDialRadius + outerRadius * cosf(angle)));
            const int y2 = static_cast<int>(lroundf(kDialRadius + outerRadius * sinf(angle)));

            canvas.drawLine(x1, y1, x2, y2, kTickWidth, color);
        }

        canvas.finishDrawing();
    }

    void setupHand(Image& hand, const lv_image_dsc_t& source, int anchor)
    {
        const int height = source.header.h;
        hand.setSrc(&source);
        hand.setPos(kCenterX - anchor, kCenterY - height / 2);
        hand.setPivot(anchor, height / 2);
        hand.removeFlag(LV_OBJ_FLAG_SCROLLABLE);
    }

    void rotateHand(Image& hand, float degrees)
    {
        hand.setRotation(static_cast<int32_t>(lroundf(degrees * 10.0f)));
    }
} // namespace

void ClassicFace::create(lv_obj_t* parent)
{
    displayMode = 0;
    lastSecond = -1;

    ticks = std::make_unique<Canvas>(parent);
    ticks->removeFlag(LV_OBJ_FLAG_SCROLLABLE);
    drawTicks(*ticks);
    ticks->align(LV_ALIGN_CENTER, 0, 0);

    dial = std::make_unique<Container>(parent);
    dial->align(LV_ALIGN_CENTER, 0, 0);
    dial->setSize(kDialSize, kDialSize);
    dial->setBgColor(lv_color_hex(kDialColor));
    dial->setBgOpa(LV_OPA_COVER);
    dial->setRadius(LV_RADIUS_CIRCLE);
    dial->setBorderWidth(0);
    dial->setOutlineWidth(0);
    dial->setShadowWidth(0);
    dial->setPaddingAll(0);
    dial->removeFlag(LV_OBJ_FLAG_SCROLLABLE);

    timeLabel = std::make_unique<Label>(parent);
    timeLabel->align(LV_ALIGN_CENTER, 0, kTimeYOffset);
    timeLabel->setTextFont(&lv_font_maple_mono_medium_28);
    timeLabel->setTextColor(lv_color_hex(kTimeColor));
    timeLabel->setBgOpa(LV_OPA_TRANSP);
    timeLabel->removeFlag(LV_OBJ_FLAG_SCROLLABLE);

    weekdayLabel = std::make_unique<Label>(parent);
    weekdayLabel->align(LV_ALIGN_CENTER, 0, kWeekdayYOffset);
    weekdayLabel->setTextFont(&lv_font_maple_mono_medium_24);
    weekdayLabel->setTextColor(lv_color_hex(kWeekdayColor));
    weekdayLabel->setBgOpa(LV_OPA_TRANSP);
    weekdayLabel->removeFlag(LV_OBJ_FLAG_SCROLLABLE);

    datePanel = std::make_unique<Container>(parent);
    datePanel->align(LV_ALIGN_CENTER, kDatePanelXOffset, 0);
    datePanel->setSize(kDatePanelWidth, kDatePanelHeight);
    datePanel->setBgColor(lv_color_hex(kDatePanelColor));
    datePanel->setBgOpa(LV_OPA_COVER);
    datePanel->setRadius(LV_RADIUS_CIRCLE);
    datePanel->setBorderWidth(0);
    datePanel->setOutlineWidth(0);
    datePanel->setShadowWidth(0);
    datePanel->setPaddingAll(0);
    datePanel->removeFlag(LV_OBJ_FLAG_SCROLLABLE);

    dateLabel = std::make_unique<Label>(datePanel->get());
    dateLabel->align(LV_ALIGN_CENTER, 0, 0);
    dateLabel->setTextFont(&lv_font_maple_mono_medium_24);
    dateLabel->setTextColor(lv_color_hex(kDateColor));
    dateLabel->setBgOpa(LV_OPA_TRANSP);
    dateLabel->removeFlag(LV_OBJ_FLAG_SCROLLABLE);

    hourHand = std::make_unique<Image>(parent);
    setupHand(*hourHand, classic_hour_hand, kHourAnchor);

    minuteHand = std::make_unique<Image>(parent);
    setupHand(*minuteHand, classic_minute_hand, kMinuteAnchor);

    secondHand = std::make_unique<Image>(parent);
    setupHand(*secondHand, classic_second_hand, kSecondAnchor);

    displayMode = 0;
    applyDisplayMode();
}

void ClassicFace::update(const ClockTime& now)
{
    if (now.second == lastSecond)
        return;
    lastSecond = now.second;

    char timeBuffer[8] = {};
    snprintf(timeBuffer, sizeof(timeBuffer), "%02d:%02d", now.hour, now.minute);

    char dateBuffer[4] = {};
    snprintf(dateBuffer, sizeof(dateBuffer), "%02d", now.day);

    if (timeLabel != nullptr)
        timeLabel->setText(timeBuffer);
    if (weekdayLabel != nullptr)
        weekdayLabel->setText(kWeekdayNames[now.weekday % 7]);
    if (dateLabel != nullptr)
        dateLabel->setText(dateBuffer);

    const float secondAngle = static_cast<float>(now.second) * 6.0f - 90.0f;
    const float minuteAngle =
        (static_cast<float>(now.minute) + static_cast<float>(now.second) / 60.0f) * 6.0f - 90.0f;
    const float hourAngle =
        ((static_cast<float>(now.hour % 12) + static_cast<float>(now.minute) / 60.0f +
          static_cast<float>(now.second) / 3600.0f) *
             30.0f) -
        90.0f;

    if (hourHand != nullptr)
        rotateHand(*hourHand, hourAngle);
    if (minuteHand != nullptr)
        rotateHand(*minuteHand, minuteAngle);
    if (secondHand != nullptr)
        rotateHand(*secondHand, secondAngle);
}

void ClassicFace::destroy()
{
    dateLabel.reset();
    datePanel.reset();
    weekdayLabel.reset();
    timeLabel.reset();
    dial.reset();
    secondHand.reset();
    minuteHand.reset();
    hourHand.reset();
    ticks.reset();
}

void ClassicFace::onTap()
{
    displayMode = (displayMode + 1) % kDisplayModeCount;
    applyDisplayMode();
}

void ClassicFace::applyDisplayMode()
{
    const bool showInfo = displayMode == 0;
    const bool showTicks = displayMode != 2;

    if (dial != nullptr)
        dial->setHidden(!showInfo);
    if (timeLabel != nullptr)
        timeLabel->setHidden(!showInfo);
    if (weekdayLabel != nullptr)
        weekdayLabel->setHidden(!showInfo);
    if (datePanel != nullptr)
        datePanel->setHidden(!showInfo);
    if (ticks != nullptr)
        ticks->setHidden(!showTicks);
}
