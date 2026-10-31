/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <array>
#include <memory>
#include <lvgl.h>
#include "lvgl/smooth_lvgl.hpp"
#include "uitk/short_namespace.hpp"

namespace view
{

    class FftView
    {
    public:
        static constexpr std::size_t bandCount = 20;
        static constexpr std::size_t reducedBandCount = 4;
        using SpectrumBands = std::array<float, bandCount>;
        using ReducedSpectrumBands = std::array<float, reducedBandCount>;

        void init(lv_obj_t* parent);
        void setSpectrum(const float* bands);
        void setPeakFrequencyHz(float frequencyHz);
        void setMicrophoneReady(bool ready);
        void update();

        const SpectrumBands& displayBands() const { return _displayBands; }
        const ReducedSpectrumBands& reducedBands() const { return _reducedBands; }
        int barRotation() const { return _barRotation; }
        int barOffset() const { return _barOffset; }
        float barBlend() const { return _barBlend; }

    private:
        std::unique_ptr<uitk::lvgl_cpp::Container> _panel;
        std::unique_ptr<uitk::lvgl_cpp::Container> _clickMask;
        std::unique_ptr<uitk::lvgl_cpp::Container> _centerDisc;
        std::unique_ptr<uitk::lvgl_cpp::Container> _spectrumPanel;
        std::unique_ptr<uitk::lvgl_cpp::Label> _peakFrequencyLabel;
        std::unique_ptr<uitk::lvgl_cpp::Label> _peakFrequencyUnitLabel;

        SpectrumBands _targetBands = {};
        SpectrumBands _displayBands = {};
        ReducedSpectrumBands _reducedBands = {};
        int _barRotation = 0;
        int _barOffset = 0;
        int _bassHitCount = 0;
        int _bassCooldown = 0;
        int _rotationDir = 1;
        float _barBlend = 0.0f;
        float _discScale = 1.0f;
        float _peakFrequencyHz = 0.0f;
        bool _micReady = false;
        bool _showCenterLabels = true;

        void toggleCenterLabels();
        void applyCenterLabelVisibility();
        void updateCenterDisc();
        void applyPeakFrequencyLabel();
        void updateReducedBands();
        void updateMotionState();
        void invalidateSpectrum();
    };

} // namespace view
