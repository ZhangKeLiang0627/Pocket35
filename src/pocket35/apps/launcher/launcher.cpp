#include "launcher.h"
#include "hal/hal.h"
#include "apps/assets/fonts/fonts.h"
#include "apps/assets/theme/theme.h"
#include <cstdlib>
#include <cstdio>

using namespace SmoothUIToolKit;
using namespace SmoothUIToolKit::SelectMenu;
using namespace MOONCAKE::APPS;
using namespace VIEW;

namespace
{
    constexpr int kIconWidth = 128;
    constexpr int kIconHeight = 128;
    constexpr int kIconGapX = 28;
    constexpr int kIconGapY = 36;
    constexpr int kIconRadius = 32;
    constexpr int kIconImageMarginTop = 14;
    constexpr int kIconImageMarginLeft = 14;
    constexpr int kIconImageWidth = 100;
    constexpr int kIconImageHeight = 100;

    constexpr int kSelectorPadding = 12;
    constexpr int kSelectorRadius = 42;

    constexpr int kPanelWidth = 254;
    constexpr int kPanelHeight = 45;
    constexpr int kPanelRadius = 18;
    constexpr int kPanelY = 262;

    constexpr int kSwipeThreshold = 60;
} // namespace

void LauncherView::init(int width, int height)
{
    _data.screenWidth = width;
    _data.screenHeight = height;

    HAL::GetCanvas()->setTextSize(1);
    HAL::GetCanvas()->setTextColor(THEME::kLauncherIconBackground);

    _data.selectorColor.jumpTo(Hex2Rgb(THEME::kLauncherBackground));
    _data.selectorColor.moveTo(Hex2Rgb(THEME::kLauncherSelector));
    _data.selectorColor.setDuration(350);

    setDuration(500);
    setTransitionPath(EasingPath::easeOutBack);

    _data.panelTransition.jumpTo(0, height);
    _data.panelTransition.moveTo(0, kPanelY);
    _data.panelTransition.setDelay(300);
    _data.panelTransition.setTransitionPath(EasingPath::easeOutBack);
    _data.panelTransition.setDuration(400);

    getCamera().setDuration(600);
    getCamera().setTransitionPath(EasingPath::easeOutBack);
    getCamera().jumpTo(-width / 2, 0);
}

void LauncherView::addAppOption(const AppOptionProps_t& props)
{
    _data.optionPropsList.push_back(props);

    const int centerOffset = (_data.screenWidth - kIconWidth) / 2;

    OptionProps_t newProps;
    newProps.keyframe.x = (_data.optionPropsList.size() - 1) * (kIconWidth + kIconGapX) + centerOffset;
    newProps.keyframe.y = kIconGapY;
    newProps.keyframe.w = kIconWidth;
    newProps.keyframe.h = kIconHeight;
    addOption(newProps);
}

void LauncherView::_update_camera_keyframe()
{
    const int centerOffset = (_data.screenWidth - kIconWidth) / 2;
    if (getSelectedKeyframe().x != getCameraOffset().x)
    {
        getCamera().moveTo(getSelectedKeyframe().x - centerOffset, 0);
    }
}

void LauncherView::handleTouch()
{
    auto& touch = HAL::GetTouchPoint();

    if (touch.isPressed && !_data.wasTouched)
    {
        _data.touchStartX = touch.x;
        _data.touchMoved = false;

        Vector4D_t squeezeShape;
        squeezeShape.w = getSelectedKeyframe().w * 3 / 4;
        squeezeShape.h = getSelectedKeyframe().h * 3 / 4;
        squeezeShape.x = getSelectedKeyframe().x + getSelectedKeyframe().w / 8;
        squeezeShape.y = getSelectedKeyframe().y + getSelectedKeyframe().h / 8;
        press(squeezeShape);
    }
    else if (touch.isPressed && _data.wasTouched)
    {
        const int16_t deltaX = touch.x - _data.touchStartX;
        if (!_data.touchMoved && std::abs(deltaX) > kSwipeThreshold)
        {
            _data.touchMoved = true;
            setConfig().moveInLoop = false;
            if (deltaX > 0)
                goLast();
            else
                goNext();
            _data.touchStartX = touch.x;
        }
    }
    else if (!touch.isPressed && _data.wasTouched)
    {
        if (!_data.touchMoved)
        {
            release();
        }
    }

    _data.wasTouched = touch.isPressed;
}

void LauncherView::onReadInput()
{
    if (isOpening())
        return;

    handleTouch();
}

void LauncherView::onUpdate(const TimeSize_t& currentTime)
{
    _data.selectorColor.update(currentTime);
    _data.panelTransition.update(currentTime);
}

void LauncherView::renderSelector()
{
    const auto frame = getSelectorCurrentFrame();
    HAL::GetCanvas()->fillSmoothRoundRect(frame.x - kSelectorPadding - getCameraOffset().x,
                                          frame.y - kSelectorPadding,
                                          frame.w + kSelectorPadding * 2,
                                          frame.h + kSelectorPadding * 2,
                                          kSelectorRadius,
                                          Rgb2Hex(_data.selectorColor.getValue()));
}

void LauncherView::renderOptions()
{
    for (size_t i = 0; i < getOptionList().size(); i++)
    {
        const auto& keyframe = getOptionList()[i].keyframe;

        HAL::GetCanvas()->fillSmoothRoundRect(keyframe.x - getCameraOffset().x,
                                              keyframe.y,
                                              keyframe.w,
                                              keyframe.h,
                                              kIconRadius,
                                              THEME::kLauncherIconBackground);

        if (_data.optionPropsList[i].icon == nullptr)
            continue;

        HAL::GetCanvas()->pushImage(keyframe.x + kIconImageMarginLeft - getCameraOffset().x,
                                    keyframe.y + kIconImageMarginTop,
                                    kIconImageWidth,
                                    kIconImageHeight,
                                    static_cast<uint16_t*>(_data.optionPropsList[i].icon));
    }
}

void LauncherView::renderPanel()
{
    const int panelX = (_data.screenWidth - kPanelWidth) / 2;
    const int panelY = _data.panelTransition.getValue().y;

    HAL::GetCanvas()->fillSmoothRoundRect(panelX, panelY, kPanelWidth, kPanelHeight, kPanelRadius, THEME::kLauncherPanel);

    if (_data.panelTransition.getYTransition().getEndValue() != _data.screenHeight)
    {
        HAL::LoadLauncherFont24();
        HAL::GetCanvas()->setTextColor(THEME::kLauncherIconBackground, THEME::kLauncherPanel);
        HAL::GetCanvas()->setTextDatum(middle_center);
        HAL::GetCanvas()->drawString(_data.optionPropsList[getSelectedOptionIndex()].name.c_str(),
                                     _data.screenWidth / 2,
                                     panelY + kPanelHeight / 2);

        const int arrowX = panelX + kPanelWidth - 34;
        const int arrowY = panelY + kPanelHeight / 2;
        HAL::GetCanvas()->fillTriangle(arrowX - 8, arrowY - 10, arrowX - 8, arrowY + 10, arrowX + 8, arrowY,
                                       THEME::kLauncherIconBackground);
    }
}

void LauncherView::renderStatusBar()
{
    auto* canvas = HAL::GetCanvas();
    canvas->setFont(&fonts::DejaVu18);
    canvas->setTextSize(1);
    canvas->setTextColor(THEME::kLauncherPanel, THEME::kLauncherBackground);

    char buffer[24];

    auto& rtc = HAL::GetRtcTime();
    snprintf(buffer, sizeof(buffer), "%02d:%02d", rtc.hour, rtc.minute);
    canvas->setTextDatum(top_left);
    canvas->drawString(buffer, 16, 6);

    auto& battery = HAL::GetBattery();
    if (battery.valid && battery.present)
    {
        snprintf(buffer, sizeof(buffer), "%s%d%%", battery.charging ? "+" : "", static_cast<int>(battery.percentage));
    }
    else if (battery.vbus)
    {
        snprintf(buffer, sizeof(buffer), "USB");
    }
    else
    {
        snprintf(buffer, sizeof(buffer), "--");
    }
    canvas->setTextDatum(top_right);
    canvas->drawString(buffer, _data.screenWidth - 16, 6);
}

void LauncherView::onRender()
{
    HAL::GetCanvas()->fillScreen(THEME::kLauncherBackground);

    renderStatusBar();

    if (isOpening())
    {
        renderOptions();
        renderPanel();
        renderSelector();
    }
    else
    {
        renderSelector();
        renderOptions();
        renderPanel();
    }

    HAL::CanvasUpdate();
}

void LauncherView::updatePanelAnim()
{
    if (getSelectedOptionIndex() == _data.lastSelectedIndex)
        return;
    _data.lastSelectedIndex = getSelectedOptionIndex();

    _data.panelTransition.setDelay(0);
    _data.panelTransition.setDuration(200);
    _data.panelTransition.moveTo(0, _data.screenHeight);
    _data.panelTransition.setUpdateCallback([this](Transition2D* transition) {
        if (transition->isFinish())
        {
            transition->setDelay(200);
            transition->moveTo(0, kPanelY);
            transition->setDuration(400);
        }
    });
}

void LauncherView::onGoLast() { updatePanelAnim(); }

void LauncherView::onGoNext() { updatePanelAnim(); }

void LauncherView::onClick()
{
    open(Vector4D_t(0 + getCameraOffset().x, 0, _data.screenWidth, _data.screenHeight));
    _data.selectorColor.jumpTo(Hex2Rgb(THEME::kLauncherSelector));
    _data.selectorColor.moveTo(Hex2Rgb(_data.optionPropsList[getSelectedOptionIndex()].themeColor));
    setDuration(350);
}

void LauncherView::onOpenEnd()
{
    if (_data.openCallback != nullptr)
    {
        _data.openCallback(_data.optionPropsList[getSelectedOptionIndex()].appID);
    }
}

void AppLauncher::onLauncherCreate() {}

void AppLauncher::onLauncherOpen()
{
    HAL::GetCanvas()->setTextScroll(false);
    createLauncherView();
}

void AppLauncher::onLauncherRunning()
{
    HAL::RtcUpdate();
    HAL::PmicUpdate();

    if (launcherView != nullptr)
    {
        launcherView->update(HAL::Millis());
    }
}

void AppLauncher::onLauncherSleeping() {}

void AppLauncher::onLauncherClose() { destroyLauncherView(); }

void AppLauncher::onLauncherDestroy() { destroyLauncherView(); }

void AppLauncher::createLauncherView()
{
    if (launcherView != nullptr)
        return;

    launcherView = new VIEW::LauncherView;
    launcherView->init(HAL::GetCanvas()->width(), HAL::GetCanvas()->height());

    for (const auto& props : getAppProps())
    {
        VIEW::LauncherView::AppOptionProps_t option;
        option.appID = props.appID;
        option.name = props.info.name;
        option.icon = props.info.icon;
        if (props.info.userData != nullptr)
        {
            option.themeColor = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(props.info.userData));
        }
        launcherView->addAppOption(option);
    }

    launcherView->setOpenCallback([this](int appID) { openApp(appID); });
}

void AppLauncher::destroyLauncherView()
{
    delete launcherView;
    launcherView = nullptr;
}
