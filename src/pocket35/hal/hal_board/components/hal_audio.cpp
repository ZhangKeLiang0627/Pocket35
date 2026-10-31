#include "../hal_config.h"
#include "../hal_board.h"
#include "boot_sound.h"
#include "driver/i2s.h"
#include "es8311.h"
#include <cmath>
#include <cstring>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace
{
    constexpr int kSampleRate = 16000;
    constexpr int kMclkMultiple = 256;
    constexpr int kFftSize = 512;
    constexpr int kHopSize = 256;
    constexpr int kBands = AUDIO::kBandCount;
    constexpr int kChunkFrames = 1024;
    constexpr int kMaxBandsPerFrame = 1;

    constexpr int kPeakMedianSize = 7;
    constexpr float kPeakMinBandLevel = 0.0010f;
    constexpr float kPeakDominance = 4.6f;
    constexpr float kPeakDeadbandHz = 12.0f;
    constexpr float kPeakFollowAlpha = 0.32f;
    constexpr int kPeakSeedFrames = 3;
    constexpr int kPeakHoldFrames = 60;

    constexpr int kSfxMaxSamples = 1024;
    constexpr int kSfxSlotCount = 3;
    constexpr int kSfxFadeSamples = 32;
    constexpr int kSfxWriteChunk = 256;
    constexpr int kSfxTaskStack = 4096;
    constexpr UBaseType_t kSfxTaskPriority = 4;
    constexpr float kSfxAmplitude = 32767.0f * 0.5f;
    constexpr float kTwoPi = 6.28318530718f;

    es8311_handle_t codec = nullptr;
    bool deviceReady = false;
    bool fftRunning = false;

    int16_t chunk[kChunkFrames * 2];
    float window[kFftSize];
    float accum[kFftSize];
    float fftRe[kFftSize];
    float fftIm[kFftSize];
    float rawBands[kBands];
    float smoothBands[kBands];
    float noiseFloor[kBands];
    float normalizationLevel = 0.002f;
    int bandEdges[kBands + 1];
    int accumCount = 0;

    float peakWindow[kPeakMedianSize];
    int peakWindowCount = 0;
    int peakWindowIndex = 0;
    float peakTrackedHz = 0.0f;
    int peakIdleFrames = 0;

    struct NoteSpec
    {
        int midi;
        float duration;
    };

    struct SfxSpec
    {
        NoteSpec notes[2];
        int noteCount;
        float gain;
    };

    constexpr SfxSpec kSfxTable[] = {
        {{{88, 0.024f}, {0, 0.0f}}, 1, 0.55f},
        {{{91, 0.020f}, {0, 0.0f}}, 1, 0.38f},
        {{{84, 0.016f}, {0, 0.0f}}, 1, 0.34f},
        {{{88, 0.030f}, {95, 0.045f}}, 2, 0.55f},
        {{{83, 0.026f}, {76, 0.040f}}, 2, 0.45f},
    };

    TaskHandle_t sfxTask = nullptr;
    int16_t sfxSlots[kSfxSlotCount][kSfxMaxSamples];
    int sfxSlotIndex = 0;
    const int16_t* sfxPendingSamples = nullptr;
    int sfxPendingCount = 0;
    portMUX_TYPE sfxLock = portMUX_INITIALIZER_UNLOCKED;

    float midiToHz(int midi) { return 440.0f * powf(2.0f, static_cast<float>(midi - 69) / 12.0f); }

    float meanOf(const float* data, int count)
    {
        float sum = 0.0f;
        for (int i = 0; i < count; i++)
            sum += data[i];
        return sum / static_cast<float>(count);
    }

    void buildWindow()
    {
        for (int i = 0; i < kFftSize; i++)
            window[i] = 0.5f * (1.0f - cosf(kTwoPi * i / (kFftSize - 1)));
    }

    void buildBandEdges()
    {
        constexpr int maxBin = kFftSize / 2;
        const float nyquist = kSampleRate * 0.5f;
        const float minHz = static_cast<float>(kSampleRate) / static_cast<float>(kFftSize);
        const float logMin = log10f(minHz);
        const float logMax = log10f(nyquist);

        bandEdges[0] = 1;
        for (int i = 1; i < kBands; i++)
        {
            const float t = static_cast<float>(i) / static_cast<float>(kBands);
            const float edgeHz = powf(10.0f, logMin + (logMax - logMin) * t);
            const int edgeBin = static_cast<int>(lroundf(edgeHz * kFftSize / kSampleRate));
            const int minEdge = bandEdges[i - 1] + 1;
            const int maxEdge = maxBin - (kBands - i);
            bandEdges[i] = edgeBin < minEdge ? minEdge : (edgeBin > maxEdge ? maxEdge : edgeBin);
        }
        bandEdges[kBands] = maxBin;
    }

    void fft(float* re, float* im)
    {
        for (int i = 1, j = 0; i < kFftSize; i++)
        {
            int bit = kFftSize >> 1;
            for (; j & bit; bit >>= 1)
                j ^= bit;
            j ^= bit;
            if (i < j)
            {
                float tr = re[i];
                re[i] = re[j];
                re[j] = tr;
                float ti = im[i];
                im[i] = im[j];
                im[j] = ti;
            }
        }

        for (int len = 2; len <= kFftSize; len <<= 1)
        {
            const float angle = -kTwoPi / static_cast<float>(len);
            const float wlenRe = cosf(angle);
            const float wlenIm = sinf(angle);
            const int half = len >> 1;

            for (int i = 0; i < kFftSize; i += len)
            {
                float wRe = 1.0f;
                float wIm = 0.0f;
                for (int j = 0; j < half; j++)
                {
                    const int a = i + j;
                    const int b = a + half;
                    const float vRe = re[b] * wRe - im[b] * wIm;
                    const float vIm = re[b] * wIm + im[b] * wRe;
                    re[b] = re[a] - vRe;
                    im[b] = im[a] - vIm;
                    re[a] += vRe;
                    im[a] += vIm;

                    const float nextRe = wRe * wlenRe - wIm * wlenIm;
                    wIm = wRe * wlenIm + wIm * wlenRe;
                    wRe = nextRe;
                }
            }
        }
    }

    float magnitudeAt(int bin)
    {
        return sqrtf(fftRe[bin] * fftRe[bin] + fftIm[bin] * fftIm[bin]) * (2.0f / static_cast<float>(kFftSize));
    }

    void resetPeakTracking()
    {
        memset(peakWindow, 0, sizeof(peakWindow));
        peakWindowCount = 0;
        peakWindowIndex = 0;
        peakTrackedHz = 0.0f;
        peakIdleFrames = 0;
    }

    float medianPeakWindow()
    {
        float sorted[kPeakMedianSize];
        memcpy(sorted, peakWindow, peakWindowCount * sizeof(float));

        for (int i = 1; i < peakWindowCount; i++)
        {
            const float key = sorted[i];
            int j = i - 1;
            for (; j >= 0 && sorted[j] > key; j--)
                sorted[j + 1] = sorted[j];
            sorted[j + 1] = key;
        }

        return sorted[peakWindowCount / 2];
    }

    float trackPeakFrequency(float candidateHz, bool hasPeak)
    {
        if (hasPeak)
        {
            peakWindow[peakWindowIndex] = candidateHz;
            peakWindowIndex = (peakWindowIndex + 1) % kPeakMedianSize;
            if (peakWindowCount < kPeakMedianSize)
                peakWindowCount++;
            peakIdleFrames = 0;
        }
        else if (peakIdleFrames < kPeakHoldFrames)
        {
            peakIdleFrames++;
        }

        if (peakIdleFrames >= kPeakHoldFrames)
        {
            peakTrackedHz = 0.0f;
            peakWindowCount = 0;
            peakWindowIndex = 0;
            return peakTrackedHz;
        }

        if (peakWindowCount < kPeakSeedFrames)
            return peakTrackedHz;

        const float median = medianPeakWindow();
        if (peakTrackedHz <= 0.0f)
        {
            peakTrackedHz = median;
        }
        else if (fabsf(median - peakTrackedHz) > kPeakDeadbandHz)
        {
            peakTrackedHz += (median - peakTrackedHz) * kPeakFollowAlpha;
        }

        return peakTrackedHz;
    }

    void processFrame(AUDIO::Spectrum_t& spectrum)
    {
        const float mean = meanOf(accum, kFftSize);
        for (int i = 0; i < kFftSize; i++)
        {
            fftRe[i] = (accum[i] - mean) * window[i];
            fftIm[i] = 0.0f;
        }

        fft(fftRe, fftIm);

        float peakMagnitude = 0.0f;
        int peakBin = 0;
        float magnitudeSum = 0.0f;
        int magnitudeCount = 0;
        float top1 = 0.0f;
        float top2 = 0.0f;
        float top3 = 0.0f;

        for (int band = 0; band < kBands; band++)
        {
            const int startBin = bandEdges[band];
            const int endBin = bandEdges[band + 1];
            float energy = 0.0f;
            float peak = 0.0f;

            for (int bin = startBin; bin < endBin; bin++)
            {
                const float mag = magnitudeAt(bin);
                magnitudeSum += mag;
                magnitudeCount++;
                if (mag > peakMagnitude)
                {
                    peakMagnitude = mag;
                    peakBin = bin;
                }
                if (mag > peak)
                    peak = mag;
                energy += mag * mag;
            }

            const int count = endBin - startBin;
            const float rms = count > 0 ? sqrtf(energy / static_cast<float>(count)) : 0.0f;
            float raw = rms * 0.48f + peak * 0.52f;
            raw *= 1.12f - 0.22f * (static_cast<float>(band) / static_cast<float>(kBands - 1));

            const float floorAlpha = raw < noiseFloor[band] ? 0.45f : 0.004f;
            noiseFloor[band] += (raw - noiseFloor[band]) * floorAlpha;
            raw -= noiseFloor[band] * 2.20f + 0.0018f;
            if (raw < 0.0f)
                raw = 0.0f;
            rawBands[band] = raw;

            if (raw >= top1)
            {
                top3 = top2;
                top2 = top1;
                top1 = raw;
            }
            else if (raw >= top2)
            {
                top3 = top2;
                top2 = raw;
            }
            else if (raw > top3)
            {
                top3 = raw;
            }
        }

        float frameReference = top1 * 0.80f + top2 * 0.14f + top3 * 0.06f;
        if (frameReference < 0.0015f)
            frameReference = 0.0015f;

        const float normAlpha = frameReference > normalizationLevel ? 0.44f : 0.16f;
        normalizationLevel += (frameReference - normalizationLevel) * normAlpha;
        if (normalizationLevel < 0.0015f)
            normalizationLevel = 0.0015f;
        if (normalizationLevel > 1.0f)
            normalizationLevel = 1.0f;

        const float meanMagnitude = magnitudeCount > 0 ? magnitudeSum / static_cast<float>(magnitudeCount) : 0.0f;
        const bool hasDominantPeak = top1 > kPeakMinBandLevel && peakMagnitude > meanMagnitude * kPeakDominance &&
                                     peakBin > 1 && peakBin < kFftSize / 2 - 1;

        float candidateHz = 0.0f;
        if (hasDominantPeak)
        {
            const float centerPower = peakMagnitude * peakMagnitude;
            const float leftPower = fftRe[peakBin - 1] * fftRe[peakBin - 1] + fftIm[peakBin - 1] * fftIm[peakBin - 1];
            const float rightPower = fftRe[peakBin + 1] * fftRe[peakBin + 1] + fftIm[peakBin + 1] * fftIm[peakBin + 1];
            const float denom = leftPower - 2.0f * centerPower + rightPower;

            float refinedBin = static_cast<float>(peakBin);
            if (fabsf(denom) > 1e-9f)
            {
                float offset = 0.5f * (leftPower - rightPower) / denom;
                if (offset < -0.5f)
                    offset = -0.5f;
                if (offset > 0.5f)
                    offset = 0.5f;
                refinedBin += offset;
            }

            candidateHz = refinedBin * static_cast<float>(kSampleRate) / static_cast<float>(kFftSize);
        }

        spectrum.peakFrequencyHz = trackPeakFrequency(candidateHz, hasDominantPeak);

        for (int band = 0; band < kBands; band++)
        {
            const float ratio = rawBands[band] / normalizationLevel;
            float normalized = powf(ratio, 0.55f);
            if (normalized > 1.0f)
                normalized = 1.0f;
            if (normalized < 0.035f)
                normalized = 0.0f;

            const float smoothAlpha = normalized > smoothBands[band] ? 0.82f : 0.40f;
            smoothBands[band] += (normalized - smoothBands[band]) * smoothAlpha;
            spectrum.bands[band] = smoothBands[band];
        }
    }

    bool openI2s()
    {
        i2s_config_t config = {};
        config.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX);
        config.sample_rate = kSampleRate;
        config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
        config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
        config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
        config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
        config.dma_buf_count = 6;
        config.dma_buf_len = 256;
        config.use_apll = false;
        config.tx_desc_auto_clear = true;
        config.fixed_mclk = 0;

        if (i2s_driver_install(I2S_NUM_0, &config, 0, nullptr) != ESP_OK)
            return false;

        i2s_pin_config_t pins = {};
        pins.mck_io_num = HAL_PIN_I2S_MCLK;
        pins.bck_io_num = HAL_PIN_I2S_BCLK;
        pins.ws_io_num = HAL_PIN_I2S_LRCK;
        pins.data_out_num = HAL_PIN_I2S_DOUT;
        pins.data_in_num = HAL_PIN_I2S_DIN;

        if (i2s_set_pin(I2S_NUM_0, &pins) != ESP_OK)
        {
            i2s_driver_uninstall(I2S_NUM_0);
            return false;
        }

        if (i2s_start(I2S_NUM_0) != ESP_OK)
        {
            i2s_driver_uninstall(I2S_NUM_0);
            return false;
        }

        return true;
    }

    bool openCodec()
    {
        codec = es8311_create(I2C_NUM_0, HAL_I2C_ADDR_ES8311);
        if (codec == nullptr)
            return false;

        const es8311_clock_config_t clkConfig = {
            .mclk_inverted = false,
            .sclk_inverted = false,
            .mclk_from_mclk_pin = true,
            .mclk_frequency = kSampleRate * kMclkMultiple,
            .sample_frequency = kSampleRate,
        };

        if (es8311_init(codec, &clkConfig, ES8311_RESOLUTION_16, ES8311_RESOLUTION_16) != ESP_OK)
        {
            es8311_delete(codec);
            codec = nullptr;
            return false;
        }

        es8311_voice_mute(codec, false);
        es8311_voice_volume_set(codec, 70, nullptr);
        es8311_microphone_config(codec, false);
        es8311_microphone_gain_set(codec, ES8311_MIC_GAIN_30DB);
        return true;
    }

    bool ensureDevice()
    {
        if (deviceReady)
            return true;

        if (!openI2s())
        {
            printf("[audio] i2s open failed\n");
            return false;
        }

        if (!openCodec())
        {
            printf("[audio] es8311 open failed\n");
            i2s_stop(I2S_NUM_0);
            i2s_driver_uninstall(I2S_NUM_0);
            return false;
        }

        deviceReady = true;
        return true;
    }

    bool writePcm(const int16_t* samples, int count)
    {
        int16_t stereo[kSfxWriteChunk * 2];
        int offset = 0;

        while (offset < count)
        {
            if (ulTaskNotifyTake(pdTRUE, 0) > 0)
                return false;

            int take = count - offset;
            if (take > kSfxWriteChunk)
                take = kSfxWriteChunk;

            for (int i = 0; i < take; i++)
            {
                const int16_t value = samples[offset + i];
                stereo[i * 2] = value;
                stereo[i * 2 + 1] = value;
            }

            size_t written = 0;
            i2s_write(I2S_NUM_0, stereo, static_cast<size_t>(take) * 2 * sizeof(int16_t), &written, portMAX_DELAY);
            offset += take;
        }

        return true;
    }

    void sfxTaskEntry(void*)
    {
        for (;;)
        {
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

            for (;;)
            {
                portENTER_CRITICAL(&sfxLock);
                const int16_t* samples = sfxPendingSamples;
                const int count = sfxPendingCount;
                sfxPendingCount = 0;
                portEXIT_CRITICAL(&sfxLock);

                if (count <= 0)
                    break;

                if (!writePcm(samples, count))
                    continue;
            }
        }
    }

    void startPlayback(const int16_t* samples, int count)
    {
        if (sfxTask == nullptr || count <= 0)
            return;

        portENTER_CRITICAL(&sfxLock);
        sfxPendingSamples = samples;
        sfxPendingCount = count;
        portEXIT_CRITICAL(&sfxLock);

        xTaskNotifyGive(sfxTask);
    }

    int synthesize(const SfxSpec& spec, int16_t* out, int maxSamples)    {
        int cursor = 0;

        for (int noteIndex = 0; noteIndex < spec.noteCount && cursor < maxSamples; noteIndex++)
        {
            const NoteSpec& note = spec.notes[noteIndex];
            const int total = static_cast<int>(static_cast<float>(kSampleRate) * note.duration);
            const float step = kTwoPi * midiToHz(note.midi) / static_cast<float>(kSampleRate);
            float angle = 0.0f;

            for (int i = 0; i < total && cursor < maxSamples; i++)
            {
                float envelope = 1.0f;
                const int remain = total - i;
                if (remain < kSfxFadeSamples)
                    envelope = static_cast<float>(remain) / static_cast<float>(kSfxFadeSamples);
                if (i < kSfxFadeSamples)
                    envelope *= static_cast<float>(i) / static_cast<float>(kSfxFadeSamples);

                out[cursor++] = static_cast<int16_t>(kSfxAmplitude * spec.gain * envelope * sinf(angle));
                angle += step;
                if (angle > kTwoPi)
                    angle -= kTwoPi;
            }
        }

        return cursor;
    }

} // namespace

void HAL_Board::audioInit()
{
    if (deviceReady)
        return;

    buildWindow();
    buildBandEdges();

    if (!ensureDevice())
        return;

    xTaskCreate(sfxTaskEntry, "audio_sfx", kSfxTaskStack, nullptr, kSfxTaskPriority, &sfxTask);
    printf("[audio] device ready %d Hz\n", kSampleRate);
}

void HAL_Board::audioOpen()
{
    memset(rawBands, 0, sizeof(rawBands));
    memset(smoothBands, 0, sizeof(smoothBands));
    memset(noiseFloor, 0, sizeof(noiseFloor));
    accumCount = 0;
    normalizationLevel = 0.002f;
    resetPeakTracking();

    if (!ensureDevice())
        return;

    if (window[0] == 0.0f)
    {
        buildWindow();
        buildBandEdges();
    }

    fftRunning = true;
    _spectrum.ready = true;
}

void HAL_Board::audioClose()
{
    fftRunning = false;
    _spectrum.ready = false;
}

void HAL_Board::audioUpdate()
{
    if (!fftRunning || !deviceReady)
        return;

    size_t bytesRead = 0;
    i2s_read(I2S_NUM_0, chunk, sizeof(chunk), &bytesRead, 0);
    if (bytesRead == 0)
        return;

    const int frames = static_cast<int>(bytesRead / (2 * sizeof(int16_t)));
    int consumed = 0;
    int processed = 0;

    while (consumed < frames && processed < kMaxBandsPerFrame)
    {
        int space = kFftSize - accumCount;
        int take = frames - consumed;
        if (take > space)
            take = space;

        for (int i = 0; i < take; i++)
        {
            const int index = (consumed + i) * 2;
            accum[accumCount + i] =
                (static_cast<float>(chunk[index]) + static_cast<float>(chunk[index + 1])) * 0.5f / 32768.0f;
        }

        accumCount += take;
        consumed += take;

        if (accumCount == kFftSize)
        {
            processFrame(_spectrum);
            processed++;

            const int keep = kFftSize - kHopSize;
            memmove(accum, accum + kHopSize, keep * sizeof(float));
            accumCount = keep;
        }
    }
}

void HAL_Board::playBootSound()
{
    if (!ensureDevice())
        return;

    startPlayback(kBootSound, kBootSoundSampleCount);
}

void HAL_Board::playSfx(AUDIO::Sfx sfx)
{
    if (!buttonConfig.sfxEnabled)
        return;

    if (!ensureDevice())
        return;

    const int index = static_cast<int>(sfx);
    constexpr int specCount = static_cast<int>(sizeof(kSfxTable) / sizeof(kSfxTable[0]));
    if (index < 0 || index >= specCount)
        return;

    int16_t* slot = sfxSlots[sfxSlotIndex];
    sfxSlotIndex = (sfxSlotIndex + 1) % kSfxSlotCount;

    const int count = synthesize(kSfxTable[index], slot, kSfxMaxSamples);
    startPlayback(slot, count);
}

void HAL_Board::setVolume(int percent)
{
    if (percent < 0)
        percent = 0;
    if (percent > 100)
        percent = 100;

    _volume = percent;

    if (codec != nullptr)
        es8311_voice_volume_set(codec, percent, nullptr);
}
