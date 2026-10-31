#include "watch_face.h"
#include "../assets/assets.h"

using namespace view;
using namespace uitk;
using namespace uitk::lvgl_cpp;

namespace
{
    constexpr int kHourTensXOffset = -42;
    constexpr int kHourTensYOffset = -51;
    constexpr int kHourOnesXOffset = 42;
    constexpr int kHourOnesYOffset = -51;
    constexpr int kMinuteTensXOffset = -42;
    constexpr int kMinuteTensYOffset = 51;
    constexpr int kMinuteOnesXOffset = 42;
    constexpr int kMinuteOnesYOffset = 51;
    constexpr int kPaletteCount = 4;

    struct Palette
    {
        uint32_t hourTens;
        uint32_t hourOnes;
        uint32_t minuteTens;
        uint32_t minuteOnes;
    };

    constexpr Palette kPalettes[kPaletteCount] = {
        {0xCBDB8C, 0xD3EA71, 0xE9F5B8, 0x94A350},
        {0xD1F3BF, 0xCDFCB9, 0xF8FEE9, 0x84D86D},
        {0xDB9D7D, 0xE8D780, 0xF4EBC0, 0xF2B050},
        {0xB49EDB, 0x9BC1FF, 0xC4C9FA, 0xAF94E7},
    };

    const lv_image_dsc_t* digitImage(int digit)
    {
        switch (digit)
        {
            case 0: return &big_number_0;
            case 1: return &big_number_1;
            case 2: return &big_number_2;
            case 3: return &big_number_3;
            case 4: return &big_number_4;
            case 5: return &big_number_5;
            case 6: return &big_number_6;
            case 7: return &big_number_7;
            case 8: return &big_number_8;
            default: return &big_number_9;
        }
    }

    void setupDigit(Image& digit, int xOffset, int yOffset)
    {
        digit.align(LV_ALIGN_CENTER, xOffset, yOffset);
        digit.removeFlag(LV_OBJ_FLAG_SCROLLABLE);
    }

    void setDigitColor(Image* image, uint32_t color)
    {
        if (image == nullptr)
            return;

        lv_obj_set_style_image_recolor(image->get(), lv_color_hex(color), 0);
        lv_obj_set_style_image_recolor_opa(image->get(), LV_OPA_COVER, 0);
    }
} // namespace

void BigNumberFace::create(lv_obj_t* parent)
{
    paletteIndex = 0;
    lastMinute = -1;

    hourTens = std::make_unique<Image>(parent);
    setupDigit(*hourTens, kHourTensXOffset, kHourTensYOffset);

    hourOnes = std::make_unique<Image>(parent);
    setupDigit(*hourOnes, kHourOnesXOffset, kHourOnesYOffset);

    minuteTens = std::make_unique<Image>(parent);
    setupDigit(*minuteTens, kMinuteTensXOffset, kMinuteTensYOffset);

    minuteOnes = std::make_unique<Image>(parent);
    setupDigit(*minuteOnes, kMinuteOnesXOffset, kMinuteOnesYOffset);

    applyPalette();
}

void BigNumberFace::update(const ClockTime& now)
{
    if (now.minute == lastMinute)
        return;
    lastMinute = now.minute;

    if (hourTens != nullptr)
        hourTens->setSrc(digitImage(now.hour / 10));
    if (hourOnes != nullptr)
        hourOnes->setSrc(digitImage(now.hour % 10));
    if (minuteTens != nullptr)
        minuteTens->setSrc(digitImage(now.minute / 10));
    if (minuteOnes != nullptr)
        minuteOnes->setSrc(digitImage(now.minute % 10));
}

void BigNumberFace::destroy()
{
    minuteOnes.reset();
    minuteTens.reset();
    hourOnes.reset();
    hourTens.reset();
}

void BigNumberFace::onTap()
{
    paletteIndex = (paletteIndex + 1) % kPaletteCount;
    applyPalette();
}

void BigNumberFace::applyPalette()
{
    const Palette& palette = kPalettes[paletteIndex];

    setDigitColor(hourTens.get(), palette.hourTens);
    setDigitColor(hourOnes.get(), palette.hourOnes);
    setDigitColor(minuteTens.get(), palette.minuteTens);
    setDigitColor(minuteOnes.get(), palette.minuteOnes);
}
