/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "app_fft.h"
#include "hal/hal.h"
#include "apps/assets/icons/icons.h"
#include "apps/assets/theme/theme.h"
#include <lvgl.h>

using namespace MOONCAKE::APPS;

void AppFft::onCreate()
{
    setAppInfo().name = "Audio FFT";
    setAppInfo().icon = (void*)image_data_icon_app_fft;
    setAppInfo().userData = (void*)(uintptr_t)THEME::kAppFftTheme;
}

void AppFft::onOpen()
{
    HAL::LvglHideCanvas();
    HAL::AudioOpen();

    fftView = std::make_unique<view::FftView>();
    fftView->init(lv_screen_active());
}

void AppFft::onRunning()
{
    if (HAL::GetButtonEvent() == BUTTON::Event::click)
    {
        close();
        return;
    }

    HAL::AudioUpdate();

    if (fftView)
    {
        auto& spectrum = HAL::GetAudioSpectrum();
        fftView->setMicrophoneReady(spectrum.ready);
        fftView->setSpectrum(spectrum.bands);
        fftView->setPeakFrequencyHz(spectrum.peakFrequencyHz);
        fftView->update();
    }
}

void AppFft::onSleeping() {}

void AppFft::onClose()
{
    fftView.reset();
    HAL::AudioClose();
    HAL::LvglShowCanvas();
}
