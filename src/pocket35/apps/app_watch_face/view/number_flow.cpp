#include "watch_face.h"
#include "../assets/assets.h"
#include <cstdio>

using namespace view;
using namespace uitk;
using namespace uitk::lvgl_cpp;

namespace
{
    constexpr int kHourXOffset = -117;
    constexpr int kMinuteXOffset = 0;
    constexpr int kSecondXOffset = 117;
    constexpr int kFlowYOffset = -22;
    constexpr int kDateYOffset = 76;
    constexpr int kPanelWidth = 100;
    constexpr int kPanelHeight = 132;
    constexpr int kPanelRadius = 42;
    constexpr int kBackgroundWidth = 480;
    constexpr int kBackgroundHeight = 320;

    constexpr const char* kWeekdayNames[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

    struct Theme
    {
        uint32_t backgroundColor;
        uint32_t panelColor;
        uint32_t timeColor;
        uint32_t dateColor;
    };

    constexpr Theme kThemes[] = {
        {0x000000, 0x1A1919, 0xFFFFFF, 0xA0A0A0},
        {0xEEEEEE, 0xE4E3E3, 0x484848, 0x525252},
        {0xC2EFEB, 0xB1E1DD, 0x566E6C, 0x6A938F},
        {0xEEE0CB, 0xD9CAB2, 0x6A5E4D, 0x948063},
        {0xB1CC74, 0xA4BD6B, 0x4B5E20, 0x668323},
        {0x8A89C0, 0x8180B4, 0xDFDEFF, 0xCBCBFF},
        {0xF26419, 0xF6722C, 0xFFEFE6, 0xFFD3BC},
        {0xF2D7EE, 0xE9C9E5, 0x83627E, 0xA4789D},
        {0x048A81, 0x05968C, 0xB1FFFA, 0x70CFC9},
        {0x9DB4C0, 0x92A8B3, 0xECF8FF, 0xDCF3FF},
    };

    constexpr int kThemeCount = static_cast<int>(sizeof(kThemes) / sizeof(kThemes[0]));

    void setupPanel(Container& panel, int xOffset, int yOffset)
    {
        panel.align(LV_ALIGN_CENTER, xOffset, yOffset);
        panel.setSize(kPanelWidth, kPanelHeight);
        panel.setRadius(kPanelRadius);
        panel.setBorderWidth(0);
        panel.setOutlineWidth(0);
        panel.setShadowWidth(0);
        panel.setPaddingAll(0);
        panel.setBgOpa(LV_OPA_COVER);
        panel.removeFlag(LV_OBJ_FLAG_SCROLLABLE);
    }

    void setupNumberFlow(NumberFlow& flow)
    {
        flow.minDigits = 2;
        flow.setAlign(LV_ALIGN_CENTER);
        flow.setPos(0, 0);
        flow.setTextFont(&CommissionerMedium64);
        flow.setTextColor(lv_color_hex(0xFFFFFF));
        flow.init();
    }
} // namespace

void NumberFlowFace::create(lv_obj_t* parent)
{
    themeIndex = 0;
    lastSecond = -1;

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

    hourPanel = std::make_unique<Container>(parent);
    setupPanel(*hourPanel, kHourXOffset, kFlowYOffset);
    hourFlow = std::make_unique<NumberFlow>(hourPanel->get());
    setupNumberFlow(*hourFlow);

    minutePanel = std::make_unique<Container>(parent);
    setupPanel(*minutePanel, kMinuteXOffset, kFlowYOffset);
    minuteFlow = std::make_unique<NumberFlow>(minutePanel->get());
    setupNumberFlow(*minuteFlow);

    secondPanel = std::make_unique<Container>(parent);
    setupPanel(*secondPanel, kSecondXOffset, kFlowYOffset);
    secondFlow = std::make_unique<NumberFlow>(secondPanel->get());
    setupNumberFlow(*secondFlow);

    dateLabel = std::make_unique<Label>(parent);
    dateLabel->align(LV_ALIGN_CENTER, 0, kDateYOffset);
    dateLabel->setTextFont(&lv_font_montserrat_24);
    dateLabel->setTextColor(lv_color_hex(0xA0A0A0));
    dateLabel->setBgOpa(LV_OPA_TRANSP);
    dateLabel->removeFlag(LV_OBJ_FLAG_SCROLLABLE);

    applyTheme();
}

void NumberFlowFace::update(const ClockTime& now)
{
    if (hourFlow != nullptr)
        hourFlow->update();
    if (minuteFlow != nullptr)
        minuteFlow->update();
    if (secondFlow != nullptr)
        secondFlow->update();

    if (now.second == lastSecond)
        return;
    lastSecond = now.second;

    if (hourFlow != nullptr)
        hourFlow->setValue(now.hour);
    if (minuteFlow != nullptr)
        minuteFlow->setValue(now.minute);
    if (secondFlow != nullptr)
        secondFlow->setValue(now.second);

    if (dateLabel != nullptr)
    {
        char buffer[32] = {};
        snprintf(buffer, sizeof(buffer), "%d/%d %s", now.month, now.day, kWeekdayNames[now.weekday % 7]);
        dateLabel->setText(buffer);
    }
}

void NumberFlowFace::destroy()
{
    dateLabel.reset();
    secondFlow.reset();
    minuteFlow.reset();
    hourFlow.reset();
    secondPanel.reset();
    minutePanel.reset();
    hourPanel.reset();
    background.reset();
}

void NumberFlowFace::onTap()
{
    themeIndex = (themeIndex + 1) % kThemeCount;
    applyTheme();
}

void NumberFlowFace::applyTheme()
{
    const Theme& theme = kThemes[themeIndex];

    if (background != nullptr)
        background->setBgColor(lv_color_hex(theme.backgroundColor));

    for (auto* panel : {hourPanel.get(), minutePanel.get(), secondPanel.get()})
    {
        if (panel != nullptr)
            panel->setBgColor(lv_color_hex(theme.panelColor));
    }

    for (auto* flow : {hourFlow.get(), minuteFlow.get(), secondFlow.get()})
    {
        if (flow != nullptr)
            flow->setDigitColor(lv_color_hex(theme.timeColor));
    }

    if (dateLabel != nullptr)
        dateLabel->setTextColor(lv_color_hex(theme.dateColor));
}
