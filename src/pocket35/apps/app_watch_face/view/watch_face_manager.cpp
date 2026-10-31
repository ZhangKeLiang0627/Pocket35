#include "watch_face.h"
#include "hal/hal.h"

using namespace view;

namespace
{
    constexpr int kScreenWidth = 480;
    constexpr int kScreenHeight = 320;
    constexpr int kSwipeMinDistance = 60;
    constexpr uint32_t kLongPressMs = 500;
    constexpr int kDotSize = 6;
    constexpr int kDotGap = 8;
    constexpr int kDotYOffset = 150;
    constexpr uint32_t kClockRefreshMs = 200;

    constexpr uint32_t kDotActiveColor = 0xF2F4F8;
    constexpr uint32_t kDotIdleColor = 0x3C4048;

    int weekdayOf(int year, int month, int day)
    {
        static const int offsets[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
        if (month < 1 || month > 12)
            return 0;

        const int adjustedYear = year - (month < 3 ? 1 : 0);
        int weekday = (adjustedYear + adjustedYear / 4 - adjustedYear / 100 + adjustedYear / 400 + offsets[month - 1] + day) % 7;
        if (weekday < 0)
            weekday += 7;
        return weekday;
    }
} // namespace

FaceManager::~FaceManager()
{
    if (currentIndex >= 0 && currentIndex < static_cast<int>(faces.size()))
        faces[currentIndex]->destroy();
}

void FaceManager::init(lv_obj_t* parent)
{
    panel = std::make_unique<uitk::lvgl_cpp::Container>(parent);
    panel->setPos(0, 0);
    panel->setSize(kScreenWidth, kScreenHeight);
    panel->setBgColor(lv_color_black());
    panel->setBgOpa(LV_OPA_COVER);
    panel->setPaddingAll(0);
    panel->setBorderWidth(0);
    panel->setOutlineWidth(0);
    panel->setShadowWidth(0);
    panel->setRadius(0);
    panel->removeFlag(LV_OBJ_FLAG_SCROLLABLE);

    faces.push_back(std::make_unique<ClassicFace>());
    faces.push_back(std::make_unique<NumberFlowFace>());
    faces.push_back(std::make_unique<BigNumberFace>());
    faces.push_back(std::make_unique<SimpleFace>());

    const int count = static_cast<int>(faces.size());
    const int dotsWidth = count * kDotSize + (count - 1) * kDotGap;

    pageDots = std::make_unique<uitk::lvgl_cpp::Container>(panel->get());
    pageDots->setSize(dotsWidth, kDotSize);
    pageDots->align(LV_ALIGN_CENTER, 0, kDotYOffset);
    pageDots->setBgOpa(LV_OPA_TRANSP);
    pageDots->setPaddingAll(0);
    pageDots->setBorderWidth(0);
    pageDots->setOutlineWidth(0);
    pageDots->setShadowWidth(0);
    pageDots->setRadius(0);
    pageDots->removeFlag(LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < count; i++)
    {
        auto dot = std::make_unique<uitk::lvgl_cpp::Container>(pageDots->get());
        dot->setSize(kDotSize, kDotSize);
        dot->setPos(i * (kDotSize + kDotGap), 0);
        dot->setRadius(LV_RADIUS_CIRCLE);
        dot->setBorderWidth(0);
        dot->setOutlineWidth(0);
        dot->setShadowWidth(0);
        dot->setPaddingAll(0);
        dot->setBgOpa(LV_OPA_COVER);
        dot->setBgColor(lv_color_hex(kDotIdleColor));
        dot->removeFlag(LV_OBJ_FLAG_SCROLLABLE);
        dotItems.push_back(std::move(dot));
    }

    if (!faces.empty())
        nextIndex = 0;
}

void FaceManager::update()
{
    if (faces.empty())
        return;

    updateClock();
    updateGesture();

    if (nextIndex >= 0 && nextIndex != currentIndex)
    {
        if (currentIndex >= 0)
            faces[currentIndex]->destroy();

        currentIndex = nextIndex;
        nextIndex = -1;

        faces[currentIndex]->create(panel->get());
        pageDots->moveForeground();
        applyPageDots();
    }

    if (currentIndex >= 0)
        faces[currentIndex]->update(clock);
}

void FaceManager::updateClock()
{
    const uint32_t now = HAL::Millis();
    if (clockValid && now - lastClockMs < kClockRefreshMs)
        return;
    lastClockMs = now;

    HAL::RtcUpdate();
    const auto& rtc = HAL::GetRtcTime();

    clock.hour = rtc.hour;
    clock.minute = rtc.minute;
    clock.second = rtc.second;
    clock.year = rtc.year;
    clock.month = rtc.month;
    clock.day = rtc.day;
    clock.weekday = weekdayOf(rtc.year, rtc.month, rtc.day);
    clockValid = true;
}

void FaceManager::updateGesture()
{
    auto& touch = HAL::GetTouchPoint();
    const uint32_t now = HAL::Millis();

    if (touch.isPressed)
    {
        if (!pressing)
        {
            pressing = true;
            pressX = static_cast<int16_t>(touch.x);
            pressY = static_cast<int16_t>(touch.y);
            pressStartMs = now;
        }

        lastX = static_cast<int16_t>(touch.x);
        lastY = static_cast<int16_t>(touch.y);
        return;
    }

    if (!pressing)
        return;

    pressing = false;

    const int deltaX = static_cast<int>(lastX - pressX);
    const int deltaY = static_cast<int>(lastY - pressY);
    const int absX = deltaX >= 0 ? deltaX : -deltaX;
    const int absY = deltaY >= 0 ? deltaY : -deltaY;

    if (absX >= kSwipeMinDistance && absX > absY)
    {
        if (deltaX < 0)
            goNext();
        else
            goPrevious();
        return;
    }

    if (currentIndex < 0)
        return;

    if (now - pressStartMs >= kLongPressMs)
        faces[currentIndex]->onLongPress();
    else
        faces[currentIndex]->onTap();
}

void FaceManager::goNext()
{
    if (faces.empty())
        return;

    const int baseIndex = currentIndex >= 0 ? currentIndex : 0;
    nextIndex = (baseIndex + 1) % static_cast<int>(faces.size());
}

void FaceManager::goPrevious()
{
    if (faces.empty())
        return;

    const int baseIndex = currentIndex >= 0 ? currentIndex : 0;
    const int count = static_cast<int>(faces.size());
    nextIndex = (baseIndex - 1 + count) % count;
}

void FaceManager::applyPageDots()
{
    for (int i = 0; i < static_cast<int>(dotItems.size()); i++)
    {
        dotItems[i]->setBgColor(lv_color_hex(i == currentIndex ? kDotActiveColor : kDotIdleColor));
    }
}
