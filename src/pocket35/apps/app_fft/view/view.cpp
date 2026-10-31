/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "view.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace
{

    void spectrumDrawEventCb(lv_event_t* e);

    using namespace view;
    using namespace uitk::lvgl_cpp;

    constexpr int _panelWidth = 480;
    constexpr int _panelHeight = 320;
    constexpr int _discSize = 112;
    constexpr int _spectrumSize = 320;
    constexpr int _extDrawSize = 0;
    constexpr int _barRestRadius = 74;
    constexpr int _barMaxRadius = 146;
    constexpr int _barColor1Stop = 74;
    constexpr int _barColor2Stop = 100;
    constexpr int _barPerBandCount = static_cast<int>(FftView::bandCount / FftView::reducedBandCount);

    constexpr uint32_t _bgColor = 0x1F1528;
    constexpr uint32_t _discColor = 0xF8C6E7;
    constexpr uint32_t _valueColor = 0xAE5D92;
    constexpr uint32_t _unitColor = 0xD586BA;
    constexpr uint32_t _barColor1 = 0xC19BFF;
    constexpr uint32_t _barColor2 = 0xE97AE0;
    constexpr uint32_t _barColor3 = 0xFF66BC;
    constexpr int _degStep = 180 / static_cast<int>(FftView::bandCount);

    constexpr float _viewAttackAlpha = 0.60f;
    constexpr float _viewReleaseAlpha = 0.36f;
    constexpr float _discFollowAlpha = 0.40f;

    constexpr std::array<int, FftView::reducedBandCount> _bandWidths = {20, 8, 4, 2};
    constexpr std::array<int, 10> _rndArray = {994, 285, 553, 11, 792, 707, 966, 641, 852, 827};

    float clampBand(float value) { return std::clamp(value, 0.0f, 1.0f); }

    int32_t getCos(int32_t deg, int32_t amplitude)
    {
        int32_t result = lv_trigo_cos(deg) * amplitude;
        result += LV_TRIGO_SIN_MAX / 2;
        return result >> LV_TRIGO_SHIFT;
    }

    int32_t getSin(int32_t deg, int32_t amplitude)
    {
        int32_t result = lv_trigo_sin(deg) * amplitude;
        return (result + LV_TRIGO_SIN_MAX / 2) >> LV_TRIGO_SHIFT;
    }

    lv_color_t mixBarColor(int radius)
    {
        if (radius < _barColor1Stop)
            return lv_color_hex(_barColor1);

        if (radius > _barMaxRadius)
            return lv_color_hex(_barColor3);

        if (radius > _barColor2Stop)
        {
            return lv_color_mix(lv_color_hex(_barColor3), lv_color_hex(_barColor2),
                                static_cast<uint8_t>(((radius - _barColor2Stop) * 255) /
                                                     (_barMaxRadius - _barColor2Stop)));
        }

        return lv_color_mix(lv_color_hex(_barColor2), lv_color_hex(_barColor1),
                            static_cast<uint8_t>(((radius - _barColor1Stop) * 255) /
                                                 (_barColor2Stop - _barColor1Stop)));
    }

} // namespace

void FftView::init(lv_obj_t* parent)
{
    _panel = std::make_unique<Container>(parent);
    _panel->align(LV_ALIGN_CENTER, 0, 0);
    _panel->setSize(_panelWidth, _panelHeight);
    _panel->setRadius(0);
    _panel->setBorderWidth(0);
    _panel->setPaddingAll(0);
    _panel->setBgColor(lv_color_hex(_bgColor));
    _panel->setBgOpa(LV_OPA_COVER);
    _panel->removeFlag(LV_OBJ_FLAG_SCROLLABLE);

    _clickMask = std::make_unique<Container>(_panel->get());
    _clickMask->align(LV_ALIGN_CENTER, 0, 0);
    _clickMask->setSize(_panelWidth, _panelHeight);
    _clickMask->setBgOpa(LV_OPA_TRANSP);
    _clickMask->setBorderWidth(0);
    _clickMask->setPaddingAll(0);
    _clickMask->removeFlag(LV_OBJ_FLAG_SCROLLABLE);
    _clickMask->onClick().connect([this]() { toggleCenterLabels(); });

    _spectrumPanel = std::make_unique<Container>(_panel->get());
    _spectrumPanel->align(LV_ALIGN_CENTER, 0, 0);
    _spectrumPanel->setSize(_spectrumSize, _spectrumSize);
    _spectrumPanel->setRadius(LV_RADIUS_CIRCLE);
    _spectrumPanel->setBorderWidth(0);
    _spectrumPanel->setPaddingAll(0);
    _spectrumPanel->setBgOpa(LV_OPA_TRANSP);
    _spectrumPanel->removeFlag(LV_OBJ_FLAG_CLICKABLE);
    _spectrumPanel->removeFlag(LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(_spectrumPanel->get(), spectrumDrawEventCb, LV_EVENT_ALL, this);
    lv_obj_refresh_ext_draw_size(_spectrumPanel->get());

    _centerDisc = std::make_unique<Container>(_panel->get());
    _centerDisc->align(LV_ALIGN_CENTER, 0, 0);
    _centerDisc->setSize(_discSize, _discSize);
    _centerDisc->setRadius(LV_RADIUS_CIRCLE);
    _centerDisc->setBorderWidth(0);
    _centerDisc->setPaddingAll(0);
    _centerDisc->setBgColor(lv_color_hex(_discColor));
    _centerDisc->setBgOpa(LV_OPA_COVER);
    _centerDisc->removeFlag(LV_OBJ_FLAG_SCROLLABLE);

    _peakFrequencyLabel = std::make_unique<Label>(_centerDisc->get());
    _peakFrequencyLabel->align(LV_ALIGN_CENTER, 0, -4);
    _peakFrequencyLabel->setTextFont(&lv_font_montserrat_24);
    _peakFrequencyLabel->setTextColor(lv_color_hex(_valueColor));
    _peakFrequencyLabel->setText("--");

    _peakFrequencyUnitLabel = std::make_unique<Label>(_centerDisc->get());
    _peakFrequencyUnitLabel->align(LV_ALIGN_CENTER, 0, 22);
    _peakFrequencyUnitLabel->setTextFont(&lv_font_montserrat_14);
    _peakFrequencyUnitLabel->setTextColor(lv_color_hex(_unitColor));
    _peakFrequencyUnitLabel->setText("Hz");

    _clickMask->moveForeground();
    applyCenterLabelVisibility();

    invalidateSpectrum();
}

void FftView::setSpectrum(const float* bands)
{
    if (bands == nullptr)
        return;

    for (std::size_t i = 0; i < bandCount; i++)
        _targetBands[i] = bands[i];
}

void FftView::setPeakFrequencyHz(float frequencyHz)
{
    if (std::fabs(frequencyHz - _peakFrequencyHz) < 1.0f)
        return;

    _peakFrequencyHz = std::max(0.0f, frequencyHz);
    applyPeakFrequencyLabel();
}

void FftView::setMicrophoneReady(bool ready)
{
    if (_micReady == ready)
        return;

    _micReady = ready;
    if (!ready)
    {
        if (_peakFrequencyLabel)
            _peakFrequencyLabel->setText("--");
        return;
    }

    applyPeakFrequencyLabel();
}

void FftView::toggleCenterLabels()
{
    _showCenterLabels = !_showCenterLabels;
    applyCenterLabelVisibility();
}

void FftView::applyCenterLabelVisibility()
{
    if (_peakFrequencyLabel)
        _peakFrequencyLabel->setHidden(!_showCenterLabels);

    if (_peakFrequencyUnitLabel)
        _peakFrequencyUnitLabel->setHidden(!_showCenterLabels);
}

void FftView::update()
{
    bool changed = false;

    for (std::size_t i = 0; i < bandCount; ++i)
    {
        float target = clampBand(_targetBands[i]);
        float current = _displayBands[i];
        float alpha = target > current ? _viewAttackAlpha : _viewReleaseAlpha;
        float smoothed = current + (target - current) * alpha;

        if (std::fabs(smoothed - current) > 0.0015f)
            changed = true;

        _displayBands[i] = smoothed;
    }

    updateReducedBands();
    updateMotionState();
    updateCenterDisc();

    if (changed)
        invalidateSpectrum();
}

void FftView::updateCenterDisc()
{
    if (_centerDisc == nullptr)
        return;

    float pulse = std::clamp(_reducedBands[0] * 0.08f + _reducedBands[1] * 0.025f, 0.0f, 0.1f);
    float targetScale = 1.0f + pulse;
    _discScale += (targetScale - _discScale) * _discFollowAlpha;

    int size = static_cast<int>(std::lround(_discSize * _discScale));
    _centerDisc->setSize(size, size);
    _centerDisc->align(LV_ALIGN_CENTER, 0, 0);
}

void FftView::applyPeakFrequencyLabel()
{
    if (_peakFrequencyLabel == nullptr)
        return;

    char buffer[16] = {};
    std::snprintf(buffer, sizeof(buffer), "%.0f", _peakFrequencyHz);
    _peakFrequencyLabel->setText(buffer);
}

void FftView::updateReducedBands()
{
    for (std::size_t band = 0; band < reducedBandCount; ++band)
    {
        std::size_t start = band * _barPerBandCount;
        float weightedSum = 0.0f;
        float weightSum = 0.0f;

        for (int i = start; i < start + _barPerBandCount; ++i)
        {
            float local = static_cast<float>(i - start) / static_cast<float>(_barPerBandCount - 1);
            float emphasis = 1.0f - local * 0.35f;
            weightedSum += _displayBands[i] * emphasis;
            weightSum += emphasis;
        }

        _reducedBands[band] = weightSum > 0.0f ? weightedSum / weightSum : 0.0f;
    }
}

void FftView::updateMotionState()
{
    _barBlend += 0.035f;
    if (_barBlend >= 1.0f)
    {
        _barBlend -= 1.0f;
        _barOffset = (_barOffset + 1) % static_cast<int>(_rndArray.size());
    }

    if (_bassCooldown > 0)
        --_bassCooldown;

    float bass = _reducedBands[0];
    if (bass > 0.84f && _bassCooldown == 0)
    {
        ++_bassHitCount;
        _bassCooldown = 14;
        if (_bassHitCount >= 3)
        {
            _bassHitCount = 0;
            _barOffset = (_barOffset + 1) % static_cast<int>(_rndArray.size());
        }
    }

    if (bass < 0.12f)
        _barRotation = (_barRotation + _rotationDir + static_cast<int>(bandCount)) % static_cast<int>(bandCount);
}

void FftView::invalidateSpectrum()
{
    if (_spectrumPanel)
        lv_obj_invalidate(_spectrumPanel->get());
}

namespace
{

    void spectrumDrawEventCb(lv_event_t* e)
    {
        auto* view = static_cast<FftView*>(lv_event_get_user_data(e));
        if (view == nullptr)
            return;

        lv_event_code_t code = lv_event_get_code(e);
        if (code == LV_EVENT_REFR_EXT_DRAW_SIZE)
        {
            lv_event_set_ext_draw_size(e, _extDrawSize);
            return;
        }

        if (code == LV_EVENT_COVER_CHECK)
        {
            lv_event_set_cover_res(e, LV_COVER_RES_NOT_COVER);
            return;
        }

        if (code != LV_EVENT_DRAW_MAIN_BEGIN)
            return;

        lv_obj_t* obj = lv_event_get_target_obj(e);
        lv_layer_t* layer = lv_event_get_layer(e);
        lv_opa_t opa = lv_obj_get_style_opa_recursive(obj, LV_PART_MAIN);
        if (opa <= LV_OPA_MIN)
            return;

        lv_area_t coords;
        lv_obj_get_coords(obj, &coords);

        lv_point_t center = {
            static_cast<lv_coord_t>(coords.x1 + lv_obj_get_width(obj) / 2),
            static_cast<lv_coord_t>(coords.y1 + lv_obj_get_height(obj) / 2),
        };

        lv_draw_triangle_dsc_t drawDsc;
        lv_draw_triangle_dsc_init(&drawDsc);
        drawDsc.opa = LV_OPA_COVER;

        std::array<int, FftView::bandCount> radii = {};
        radii.fill(_barRestRadius);

        for (std::size_t s = 0; s < FftView::reducedBandCount; ++s)
        {
            int bandWidth = _bandWidths[s];
            int amplitude =
                static_cast<int>((_barMaxRadius - _barRestRadius) * clampBand(view->reducedBands()[s]));

            for (int f = 0; f < bandWidth; ++f)
            {
                int32_t amplMod = getCos(f * 360 / bandWidth + 180, 180) + 180;
                int t = _barPerBandCount * static_cast<int>(s) - bandWidth / 2 + f;
                if (t < 0)
                    t += static_cast<int>(FftView::bandCount);
                if (t >= static_cast<int>(FftView::bandCount))
                    t -= static_cast<int>(FftView::bandCount);

                radii[t] += (amplitude * amplMod) >> 9;
            }
        }

        for (std::size_t i = 0; i < FftView::bandCount; ++i)
        {
            int j = (static_cast<int>(i) + view->barRotation() +
                     _rndArray[view->barOffset() % static_cast<int>(_rndArray.size())]) %
                    static_cast<int>(FftView::bandCount);
            int k = (static_cast<int>(i) + view->barRotation() +
                     _rndArray[(view->barOffset() + 1) % static_cast<int>(_rndArray.size())]) %
                    static_cast<int>(FftView::bandCount);
            int radius = static_cast<int>(radii[k] * view->barBlend() + radii[j] * (1.0f - view->barBlend()));
            drawDsc.color = mixBarColor(radius);

            int32_t degSpace = 1;
            int32_t deg = static_cast<int32_t>(i) * _degStep + 90;
            int32_t outerDegA = deg + degSpace;
            int32_t outerDegB = deg + _degStep - degSpace;

            int32_t x1Out = getCos(outerDegA, radius);
            int32_t y1Out = getSin(outerDegA, radius);
            int32_t x2Out = getCos(outerDegB, radius);
            int32_t y2Out = getSin(outerDegB, radius);
            int32_t xIn = getCos(outerDegB, 0);
            int32_t yIn = getSin(outerDegB, 0);

            drawDsc.p[0].x = center.x + x1Out;
            drawDsc.p[0].y = center.y + y1Out;
            drawDsc.p[1].x = center.x + x2Out;
            drawDsc.p[1].y = center.y + y2Out;
            drawDsc.p[2].x = center.x + xIn;
            drawDsc.p[2].y = center.y + yIn;
            lv_draw_triangle(layer, &drawDsc);

            drawDsc.p[0].x = center.x - x1Out;
            drawDsc.p[1].x = center.x - x2Out;
            drawDsc.p[2].x = center.x - xIn;
            lv_draw_triangle(layer, &drawDsc);
        }
    }

} // namespace
