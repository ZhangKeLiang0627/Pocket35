#include "launcher.h"
#include "hal/hal.h"
#include "apps/assets/fonts/fonts.h"
#include "apps/assets/theme/theme.h"
#include <cstdlib>
#include <cmath>
#include <cstdio>
#include <cstring>

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

    constexpr int kIconPitch = kIconWidth + kIconGapX;
    constexpr int kSwipeStepPixels = kIconPitch;
    constexpr int kDragSlop = 12;

    constexpr float kSwipeVelocitySmoothing = 0.5f;
    constexpr float kFlingProjectMs = 170.0f;
    constexpr int kFlingMaxSteps = 6;
    constexpr int kMoveDuration = 500;
    constexpr int kCameraMoveDuration = 600;

    constexpr int kPanelHideDuration = 200;
    constexpr int kPanelShowDelay = 200;
    constexpr int kPanelShowDuration = 400;

    constexpr int kStripBandTop = 24;
    constexpr int kStripBandBottom = 175;
    constexpr int kPanelBandTop = 254;
    constexpr int kStatusBandBottom = 34;

    constexpr uint16_t rgb565Of(uint32_t rgb888)
    {
        return static_cast<uint16_t>((((rgb888 >> 19) & 0x1F) << 11) | (((rgb888 >> 10) & 0x3F) << 5) |
                                     ((rgb888 >> 3) & 0x1F));
    }

    constexpr uint16_t swapped16(uint16_t value) { return static_cast<uint16_t>((value >> 8) | (value << 8)); }

    constexpr uint16_t kBackgroundWord = swapped16(rgb565Of(THEME::kLauncherBackground));

    struct Band_t
    {
        int top;
        int bottom;
    };

    void buildStatusText(char* buffer, size_t size)
    {
        auto& rtc = HAL::GetRtcTime();
        auto& battery = HAL::GetBattery();

        char timeBuf[16];
        snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", rtc.hour, rtc.minute);

        char battBuf[16];
        if (battery.valid && battery.present)
        {
            snprintf(battBuf, sizeof(battBuf), "%s%d%%", battery.charging ? "+" : "", static_cast<int>(battery.percentage));
        }
        else if (battery.vbus)
        {
            snprintf(battBuf, sizeof(battBuf), "USB");
        }
        else
        {
            snprintf(battBuf, sizeof(battBuf), "--");
        }

        snprintf(buffer, size, "%s|%s", timeBuf, battBuf);
    }
} // namespace

LauncherView::~LauncherView()
{
    for (auto* card : _data.cardCache)
    {
        delete card;
    }
    _data.cardCache.clear();
}

void LauncherView::init(int width, int height)
{
    _data.screenWidth = width;
    _data.screenHeight = height;

    HAL::GetCanvas()->setTextSize(1);
    HAL::GetCanvas()->setTextColor(THEME::kLauncherIconBackground);

    _data.selectorColor.jumpTo(Hex2Rgb(THEME::kLauncherBackground));
    _data.selectorColor.moveTo(Hex2Rgb(THEME::kLauncherSelector));
    _data.selectorColor.setDuration(350);

    setDuration(kMoveDuration);
    setTransitionPath(EasingPath::easeOutBack);

    _data.panelTransition.jumpTo(0, height);
    _data.panelTransition.moveTo(0, kPanelY);
    _data.panelTransition.setDelay(300);
    _data.panelTransition.setTransitionPath(EasingPath::easeOutBack);
    _data.panelTransition.setDuration(kPanelShowDuration);

    getCamera().setDuration(kCameraMoveDuration);
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

    _buildCardCache(_data.optionPropsList.size() - 1);
}

void LauncherView::_buildCardCache(size_t index)
{
    auto* card = new lgfx::LGFX_Sprite(HAL::GetCanvas());
    card->setPsram(true);
    card->setColorDepth(HAL::GetCanvas()->getColorDepth());
    if (card->createSprite(kIconWidth, kIconHeight) == nullptr)
    {
        delete card;
        _data.cardCache.push_back(nullptr);
        return;
    }

    card->fillScreen(THEME::kLauncherBackground);
    card->fillSmoothRoundRect(0, 0, kIconWidth, kIconHeight, kIconRadius, THEME::kLauncherIconBackground);

    void* icon = _data.optionPropsList[index].icon;
    if (icon != nullptr)
    {
        card->pushImage(kIconImageMarginLeft, kIconImageMarginTop, kIconImageWidth, kIconImageHeight,
                        static_cast<uint16_t*>(icon));
    }

    _data.cardCache.push_back(card);
}

void LauncherView::_clearRows(int y, int height)
{
    if (height <= 0)
        return;

    auto* row = static_cast<uint16_t*>(HAL::GetCanvas()->getBuffer()) + static_cast<size_t>(y) * _data.screenWidth;
    auto* word = reinterpret_cast<uint32_t*>(row);
    const uint32_t pattern = (static_cast<uint32_t>(kBackgroundWord) << 16) | kBackgroundWord;

    size_t count = static_cast<size_t>(_data.screenWidth) * height / 2;
    while (count--)
    {
        *word++ = pattern;
    }
}

void LauncherView::setSelectedOptionIndex(int index)
{
    jumpTo(index);

    const int centerOffset = (_data.screenWidth - kIconWidth) / 2;
    getCamera().jumpTo(getSelectedKeyframe().x - centerOffset, 0);
}

int LauncherView::_cameraX() { return getCameraOffset().x - _data.dragOffset; }

bool LauncherView::isTouchInsideIcon(int16_t x, int16_t y)
{
    const auto& keyframe = getSelectedKeyframe();
    const int screenX = keyframe.x - _cameraX();
    const int screenY = keyframe.y;
    return x >= screenX && x <= screenX + keyframe.w && y >= screenY && y <= screenY + keyframe.h;
}

void LauncherView::_update_camera_keyframe()
{
    const int centerOffset = (_data.screenWidth - kIconWidth) / 2;
    if (getSelectedKeyframe().x != getCameraOffset().x)
    {
        getCamera().moveTo(getSelectedKeyframe().x - centerOffset, 0);
    }
}

void LauncherView::_trackSwipeVelocity(int16_t x, TimeSize_t now)
{
    const TimeSize_t deltaTime = now - _data.lastTouchTime;
    if (deltaTime == 0)
        return;

    const float instantVelocity = static_cast<float>(x - _data.lastTouchX) / static_cast<float>(deltaTime);
    _data.swipeVelocity += (instantVelocity - _data.swipeVelocity) * kSwipeVelocitySmoothing;

    _data.lastTouchX = x;
    _data.lastTouchTime = now;
}

void LauncherView::_releaseDrag()
{
    const float flingPixels = _data.swipeVelocity * kFlingProjectMs;
    const float totalPixels = static_cast<float>(_data.dragOffset) + flingPixels;
    int steps = static_cast<int>(std::lround(-totalPixels / static_cast<float>(kSwipeStepPixels)));
    if (steps > kFlingMaxSteps)
        steps = kFlingMaxSteps;
    if (steps < -kFlingMaxSteps)
        steps = -kFlingMaxSteps;

    const int lastIndex = static_cast<int>(_data.optionPropsList.size()) - 1;
    int target = getSelectedOptionIndex() + steps;
    if (target < 0)
        target = 0;
    if (target > lastIndex)
        target = lastIndex;

    getCamera().jumpTo(getCameraOffset().x - _data.dragOffset, 0);
    _data.dragOffset = 0;

    setConfig().moveInLoop = false;
    moveTo(target);
    _update_camera_keyframe();
    updatePanelAnim();
}

void LauncherView::handleTouch()
{
    auto& touch = HAL::GetTouchPoint();
    const TimeSize_t now = HAL::Millis();

    if (touch.isPressed && !_data.wasTouched)
    {
        _data.touchStartX = touch.x;
        _data.dragOffset = 0;
        _data.isDragging = false;
        _data.lastTouchX = touch.x;
        _data.lastTouchTime = now;
        _data.swipeVelocity = 0.0f;
        _data.touchOnIcon = isTouchInsideIcon(touch.x, touch.y);

        if (_data.touchOnIcon)
        {
            Vector4D_t squeezeShape;
            squeezeShape.w = getSelectedKeyframe().w * 3 / 4;
            squeezeShape.h = getSelectedKeyframe().h * 3 / 4;
            squeezeShape.x = getSelectedKeyframe().x + getSelectedKeyframe().w / 8;
            squeezeShape.y = getSelectedKeyframe().y + getSelectedKeyframe().h / 8;
            press(squeezeShape);
        }
    }
    else if (touch.isPressed && _data.wasTouched)
    {
        _trackSwipeVelocity(touch.x, now);

        const int16_t offset = touch.x - _data.touchStartX;
        if (!_data.isDragging && std::abs(offset) > kDragSlop)
        {
            _data.isDragging = true;
            if (isPressing())
                moveTo(getSelectedOptionIndex());
        }

        if (_data.isDragging)
        {
            _data.dragOffset = offset;
            setConfig().moveInLoop = false;
        }
    }
    else if (!touch.isPressed && _data.wasTouched)
    {
        if (_data.isDragging)
        {
            _releaseDrag();
        }
        else if (_data.touchOnIcon && isTouchInsideIcon(touch.x, touch.y))
        {
            release();
        }

        _data.touchOnIcon = false;
        _data.isDragging = false;
        _data.dragOffset = 0;
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
    HAL::GetCanvas()->fillSmoothRoundRect(frame.x - kSelectorPadding - _cameraX(),
                                          frame.y - kSelectorPadding,
                                          frame.w + kSelectorPadding * 2,
                                          frame.h + kSelectorPadding * 2,
                                          kSelectorRadius,
                                          Rgb2Hex(_data.selectorColor.getValue()));
}

void LauncherView::_renderCardAt(size_t index, int cameraX, const Vector4D_t& selectorFrame)
{
    if (index >= _data.cardCache.size())
        return;

    auto* card = _data.cardCache[index];
    if (card == nullptr)
        return;

    const auto& keyframe = getOptionList()[index].keyframe;
    const int x = keyframe.x - cameraX;
    const int y = keyframe.y;
    if (x + kIconWidth <= 0 || x >= _data.screenWidth)
        return;

    auto* canvas = HAL::GetCanvas();

    const int selectorLeft = selectorFrame.x - kSelectorPadding - cameraX;
    const int selectorTop = selectorFrame.y - kSelectorPadding;
    const int selectorRight = selectorLeft + selectorFrame.w + kSelectorPadding * 2;
    const int selectorBottom = selectorTop + selectorFrame.h + kSelectorPadding * 2;
    const bool behindSelector =
        x < selectorRight && selectorLeft < x + kIconWidth && y < selectorBottom && selectorTop < y + kIconHeight;

    if (!behindSelector)
    {
        card->pushSprite(canvas, x, y);
        return;
    }

    canvas->fillSmoothRoundRect(x, y, kIconWidth, kIconHeight, kIconRadius, THEME::kLauncherIconBackground);
    if (_data.optionPropsList[index].icon != nullptr)
    {
        canvas->pushImage(x + kIconImageMarginLeft,
                          y + kIconImageMarginTop,
                          kIconImageWidth,
                          kIconImageHeight,
                          static_cast<uint16_t*>(_data.optionPropsList[index].icon));
    }
}

void LauncherView::renderOptions()
{
    const int cameraX = _cameraX();
    const auto selectorFrame = getSelectorCurrentFrame();
    for (size_t i = 0; i < _data.optionPropsList.size(); i++)
    {
        _renderCardAt(i, cameraX, selectorFrame);
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
    const bool stripMoving = isOpening() || _data.isDragging || !_data.selectorColor.isFinish() ||
                             !getSelectorPostion().isFinish() || !getSelectorShape().isFinish() ||
                             !getCamera().isFinish();
    const bool panelMoving = !_data.panelTransition.isFinish();
    const bool animating = stripMoving || panelMoving;

    char statusText[32];
    buildStatusText(statusText, sizeof(statusText));
    const bool statusChanged = strcmp(statusText, _data.lastStatusText) != 0;

    if (!animating && !_data.isDirty && !statusChanged && !_data.forceFullRedraw && !_data.wasAnimating)
    {
        return;
    }

    _data.isDirty = false;
    _data.wasAnimating = animating;

    Band_t bands[3];
    int bandCount = 0;
    const auto addBand = [&bands, &bandCount](int top, int bottom) {
        if (top > bottom)
            return;
        for (int i = 0; i < bandCount; i++)
        {
            if (top <= bands[i].bottom + 1 && bottom + 1 >= bands[i].top)
            {
                bands[i].top = top < bands[i].top ? top : bands[i].top;
                bands[i].bottom = bottom > bands[i].bottom ? bottom : bands[i].bottom;
                return;
            }
        }
        if (bandCount < 3)
        {
            bands[bandCount].top = top;
            bands[bandCount].bottom = bottom;
            bandCount++;
        }
    };

    const int panelY = _data.panelTransition.getValue().y;
    const bool fullRedraw = _data.forceFullRedraw || isOpening();

    if (fullRedraw)
    {
        addBand(0, _data.screenHeight - 1);
    }
    else
    {
        if (stripMoving)
            addBand(kStripBandTop, kStripBandBottom);
        if (panelMoving)
        {
            const int top = panelY < _data.lastPanelY ? panelY : _data.lastPanelY;
            addBand(top < kPanelBandTop ? kPanelBandTop : top, _data.screenHeight - 1);
        }
        if (statusChanged)
        {
            addBand(0, kStatusBandBottom);
            addBand(kStripBandTop, kStripBandBottom);
        }
    }

    _data.forceFullRedraw = false;
    _data.lastPanelY = panelY;

    if (bandCount == 0)
    {
        return;
    }

    for (int i = 0; i < bandCount; i++)
    {
        _clearRows(bands[i].top, bands[i].bottom - bands[i].top + 1);
    }

    if (fullRedraw || statusChanged)
    {
        renderStatusBar();
        strncpy(_data.lastStatusText, statusText, sizeof(_data.lastStatusText) - 1);
        _data.lastStatusText[sizeof(_data.lastStatusText) - 1] = '\0';
    }

    if (fullRedraw || stripMoving || statusChanged)
    {
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
        }
    }

    if (!isOpening() && (fullRedraw || panelMoving))
    {
        renderPanel();
    }

    for (int i = 0; i < bandCount; i++)
    {
        HAL::CanvasPush(0, bands[i].top, _data.screenWidth, bands[i].bottom - bands[i].top + 1);
    }
}

void LauncherView::updatePanelAnim()
{
    if (getSelectedOptionIndex() == _data.lastSelectedIndex)
        return;
    _data.lastSelectedIndex = getSelectedOptionIndex();

    _data.panelTransition.setDelay(0);
    _data.panelTransition.setDuration(kPanelHideDuration);
    _data.panelTransition.moveTo(0, _data.screenHeight);
    _data.panelTransition.setUpdateCallback([this](Transition2D* transition) {
        if (transition->isFinish())
        {
            transition->setDelay(kPanelShowDelay);
            transition->moveTo(0, kPanelY);
            transition->setDuration(kPanelShowDuration);
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
    HAL::LvglFlushNow();
    createLauncherView();

    if (launcherView != nullptr)
    {
        launcherView->renderNow();
    }
}

void AppLauncher::onLauncherRunning()
{
    static unsigned long lastSensorMs = 0;
    const unsigned long now = HAL::Millis();
    if (now - lastSensorMs >= 1000)
    {
        lastSensorMs = now;
        HAL::RtcUpdate();
        HAL::PmicUpdate();
    }

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

    int restoreIndex = -1;
    int index = 0;
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
        if (props.appID == lastSelectedAppId)
        {
            restoreIndex = index;
        }
        index++;
    }

    launcherView->setOpenCallback([this](int appID) {
        lastSelectedAppId = appID;
        openApp(appID);
    });

    if (restoreIndex >= 0)
    {
        launcherView->setSelectedOptionIndex(restoreIndex);
    }
}

void AppLauncher::destroyLauncherView()
{
    delete launcherView;
    launcherView = nullptr;
}
