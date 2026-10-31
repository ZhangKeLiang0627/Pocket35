#include "watch_face.h"
#include "../assets/assets.h"
#include "core/animation/animate_value/animate_value.hpp"
#include "hal/hal.h"
#include <cmath>
#include <cstdio>

using namespace view;
using namespace uitk;
using namespace uitk::lvgl_cpp;

namespace
{
    constexpr int kBackgroundWidth = 480;
    constexpr int kBackgroundHeight = 320;
    constexpr int kTimeYOffset = -24;
    constexpr int kDateYOffset = 57;
    constexpr int kTipsYOffset = 118;
    constexpr int kOrbitBallSize = 14;
    constexpr int kOrbitBallRadius = 140;
    constexpr uint32_t kTipsDurationMs = 3000;
    constexpr float kPi = 3.14159265358979323846f;

    constexpr const char* kWeekdayNames[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

    struct Theme
    {
        uint32_t backgroundColor;
        uint32_t ballColor;
        uint32_t timeColor;
        uint32_t dateColor;
    };

    constexpr Theme kThemes[] = {
        {0x000000, 0xA0A0A0, 0xFFFFFF, 0xA0A0A0},
        {0xEEEEEE, 0x525252, 0x484848, 0x525252},
        {0xC2EFEB, 0x6A938F, 0x566E6C, 0x6A938F},
        {0xEEE0CB, 0x948063, 0x6A5E4D, 0x948063},
        {0xB1CC74, 0x668323, 0x4B5E20, 0x668323},
        {0x8A89C0, 0xCBCBFF, 0xDFDEFF, 0xCBCBFF},
        {0xF26419, 0xFFD3BC, 0xFFEFE6, 0xFFD3BC},
        {0xF2D7EE, 0xA4789D, 0x83627E, 0xA4789D},
        {0x048A81, 0x70CFC9, 0xB1FFFA, 0x70CFC9},
        {0x9DB4C0, 0xDCF3FF, 0xECF8FF, 0xDCF3FF},
    };

    constexpr int kThemeCount = static_cast<int>(sizeof(kThemes) / sizeof(kThemes[0]));

    float unwrapAngle(float previousUnwrapped, float nextAngle)
    {
        float previousAngle = fmodf(previousUnwrapped, 360.0f);
        if (previousAngle < 0.0f)
            previousAngle += 360.0f;

        float delta = fmodf(nextAngle - previousAngle, 360.0f);
        if (delta < -180.0f)
            delta += 360.0f;
        else if (delta > 180.0f)
            delta -= 360.0f;

        return previousUnwrapped + delta;
    }
} // namespace

void SimpleFace::create(lv_obj_t* parent)
{
    themeIndex = 0;
    lastSecond = -1;
    showSecondBall = true;
    showTips = true;
    orbitInitialized = false;
    orbitUnwrapped = 0.0f;

    orbitAngle.pause();
    orbitAngle.springOptions().visualDuration = 0.3f;
    orbitAngle.springOptions().bounce = 0.4f;
    orbitAngle.teleport(0.0f);
    orbitAngle.play();

    background = std::make_unique<Container>(parent);
    background->align(LV_ALIGN_CENTER, 0, 0);
    background->setSize(kBackgroundWidth, kBackgroundHeight);
    background->setRadius(0);
    background->setBorderWidth(0);
    background->setOutlineWidth(0);
    background->setShadowWidth(0);
    background->setPaddingAll(0);
    background->setBgOpa(LV_OPA_COVER);
    background->removeFlag(LV_OBJ_FLAG_SCROLLABLE);

    timeLabel = std::make_unique<Label>(parent);
    timeLabel->align(LV_ALIGN_CENTER, 0, kTimeYOffset);
    timeLabel->setTextFont(&CommissionerMedium108);
    timeLabel->setBgOpa(LV_OPA_TRANSP);
    timeLabel->removeFlag(LV_OBJ_FLAG_SCROLLABLE);

    dateLabel = std::make_unique<Label>(parent);
    dateLabel->align(LV_ALIGN_CENTER, 0, kDateYOffset);
    dateLabel->setTextFont(&lv_font_montserrat_24);
    dateLabel->setBgOpa(LV_OPA_TRANSP);
    dateLabel->removeFlag(LV_OBJ_FLAG_SCROLLABLE);

    tipsLabel = std::make_unique<Label>(parent);
    tipsLabel->align(LV_ALIGN_CENTER, 0, kTipsYOffset);
    tipsLabel->setTextFont(&lv_font_montserrat_14);
    tipsLabel->setBgOpa(LV_OPA_TRANSP);
    tipsLabel->removeFlag(LV_OBJ_FLAG_SCROLLABLE);
    tipsLabel->setText("long press to hide\nthe second dot");
    tipsLabel->setTextColor(lv_color_hex(0xEAE48D));
    tipsLabel->setTextAlign(LV_TEXT_ALIGN_CENTER);

    secondBall = std::make_unique<Container>(parent);
    secondBall->setSize(kOrbitBallSize, kOrbitBallSize);
    secondBall->setBgOpa(LV_OPA_COVER);
    secondBall->setBorderWidth(0);
    secondBall->setOutlineWidth(0);
    secondBall->setShadowWidth(0);
    secondBall->setPaddingAll(0);
    secondBall->setRadius(LV_RADIUS_CIRCLE);
    secondBall->removeFlag(LV_OBJ_FLAG_SCROLLABLE);

    applyTheme();
    tipsHideTick = HAL::Millis() + kTipsDurationMs;
    applySecondBall();
}

void SimpleFace::update(const ClockTime& now)
{
    if (tipsLabel != nullptr && showTips && HAL::Millis() >= tipsHideTick)
    {
        tipsLabel->setHidden(true);
        showTips = false;
    }

    if (!orbitAngle.done())
    {
        orbitAngle.update();
        applySecondBall();
    }

    if (now.second == lastSecond)
        return;
    lastSecond = now.second;

    if (timeLabel != nullptr)
    {
        char buffer[8] = {};
        snprintf(buffer, sizeof(buffer), "%02d:%02d", now.hour, now.minute);
        timeLabel->setText(buffer);
    }

    if (dateLabel != nullptr)
    {
        char buffer[32] = {};
        snprintf(buffer, sizeof(buffer), "%d/%d %s", now.month, now.day, kWeekdayNames[now.weekday % 7]);
        dateLabel->setText(buffer);
    }

    setOrbitAngle(static_cast<float>(now.second * 6));
    applySecondBall();
}

void SimpleFace::destroy()
{
    secondBall.reset();
    tipsLabel.reset();
    dateLabel.reset();
    timeLabel.reset();
    background.reset();
}

void SimpleFace::onTap()
{
    themeIndex = (themeIndex + 1) % kThemeCount;
    applyTheme();
}

void SimpleFace::onLongPress()
{
    showSecondBall = !showSecondBall;
    applySecondBall();
}

void SimpleFace::applySecondBall()
{
    if (secondBall == nullptr)
        return;

    secondBall->setHidden(!showSecondBall);
    if (!showSecondBall)
        return;

    const float radians = (static_cast<float>(orbitAngle) - 90.0f) * kPi / 180.0f;
    const int offsetX = static_cast<int>(lroundf(kOrbitBallRadius * cosf(radians)));
    const int offsetY = static_cast<int>(lroundf(kOrbitBallRadius * sinf(radians)));
    secondBall->align(LV_ALIGN_CENTER, offsetX, offsetY);
}

void SimpleFace::setOrbitAngle(float angle)
{
    if (!orbitInitialized)
    {
        orbitUnwrapped = angle;
        orbitInitialized = true;
    }
    else
    {
        orbitUnwrapped = unwrapAngle(orbitUnwrapped, angle);
    }

    orbitAngle = orbitUnwrapped;
}

void SimpleFace::applyTheme()
{
    const Theme& theme = kThemes[themeIndex];

    if (background != nullptr)
        background->setBgColor(lv_color_hex(theme.backgroundColor));

    if (timeLabel != nullptr)
        timeLabel->setTextColor(lv_color_hex(theme.timeColor));

    if (dateLabel != nullptr)
        dateLabel->setTextColor(lv_color_hex(theme.dateColor));

    if (secondBall != nullptr)
        secondBall->setBgColor(lv_color_hex(theme.ballColor));
}
