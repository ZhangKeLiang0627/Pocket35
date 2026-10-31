#include "view.h"
#include "hal/hal.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <core/color/color.hpp>
#include <core/easing/ease.hpp>
#include <string>
#include <tools/random/random.hpp>

LV_IMAGE_DECLARE(lucky_wheel_pointer);

using namespace view;
using namespace uitk;
using namespace uitk::lvgl_cpp;

namespace
{

    void wheelDrawEventCb(lv_event_t* e);

    constexpr int kPanelWidth = 480;
    constexpr int kPanelHeight = 320;
    constexpr int kPanelCenterX = kPanelWidth / 2;
    constexpr int kPanelCenterY = kPanelHeight / 2;
    constexpr int kWheelOuterRadius = 128;
    constexpr int kWheelWidth = kWheelOuterRadius * 2;
    constexpr int kPointerAnchorUp = 47;
    constexpr uint32_t kSpinMinDurationMs = 2400;
    constexpr uint32_t kSpinMaxDurationMs = 3600;
    constexpr int kSpinMinTurns = 4;
    constexpr int kSpinMaxTurns = 7;
    constexpr float kSectorEdgeSafeRatio = 0.22f;
    constexpr float kSectorEdgeSafeMaxDeg = 8.0f;
    constexpr int kLabelRadius = 92;
    constexpr int kLabelWidth = 72;
    constexpr int kLabelHeight = 28;
    constexpr uint32_t kBgColor = 0x101216;
    constexpr uint32_t kLabelFg = 0x4C4C4C;
    constexpr uint32_t kPointerScale = 140;

    constexpr std::array<uint32_t, 5> kSectorColors = {
        0x4AD78C, 0x7AC4F5, 0xF4CA63, 0xD194EA, 0xFF77A0,
    };

    std::vector<int> buildColorOrder(int optionCount)
    {
        std::vector<int> order;
        order.reserve(optionCount);

        std::array<int, kSectorColors.size()> usage = {};

        for (int i = 0; i < optionCount; ++i)
        {
            int bestIndex = 0;
            int bestScore = -1000000;

            for (int colorIndex = 0; colorIndex < static_cast<int>(kSectorColors.size()); ++colorIndex)
            {
                if (!order.empty() && order.back() == colorIndex)
                {
                    continue;
                }

                if (i == optionCount - 1 && !order.empty() && order.front() == colorIndex)
                {
                    continue;
                }

                int score = -usage[colorIndex] * 100;

                if (i > 0)
                {
                    int prev = order.back();
                    int circularDistance = std::abs(colorIndex - prev);
                    circularDistance = std::min(circularDistance, static_cast<int>(kSectorColors.size()) - circularDistance);
                    score += circularDistance * 10;
                }

                if (score > bestScore)
                {
                    bestScore = score;
                    bestIndex = colorIndex;
                }
            }

            order.push_back(bestIndex);
            usage[bestIndex]++;
        }

        return order;
    }

    float normalizeDegrees(float degrees)
    {
        float normalized = std::fmod(degrees, 360.0f);
        if (normalized < 0.0f)
        {
            normalized += 360.0f;
        }
        return normalized;
    }

    float computeSignedSpinTarget(float currentDeg, float targetDeg, int fullTurns, SpinDirection direction)
    {
        float currentNorm = normalizeDegrees(currentDeg);
        float targetNorm = normalizeDegrees(targetDeg);

        if (direction == SpinDirection::Clockwise)
        {
            float delta = normalizeDegrees(targetNorm - currentNorm);
            return currentDeg + static_cast<float>(fullTurns * 360) + delta;
        }

        float delta = normalizeDegrees(currentNorm - targetNorm);
        return currentDeg - static_cast<float>(fullTurns * 360) - delta;
    }

} // namespace

void WheelView::init(lv_obj_t* parent, int optionCount)
{
    _option_count = std::max(2, optionCount);

    _panel = std::make_unique<Container>(parent);
    _panel->align(LV_ALIGN_CENTER, 0, 0);
    _panel->setSize(kPanelWidth, kPanelHeight);
    _panel->setRadius(0);
    _panel->setBorderWidth(0);
    _panel->setPaddingAll(0);
    _panel->setBgColor(lv_color_hex(kBgColor));
    _panel->setBgOpa(LV_OPA_COVER);
    _panel->removeFlag(LV_OBJ_FLAG_SCROLLABLE);

    _wheel_obj = std::make_unique<Container>(_panel->get());
    _wheel_obj->align(LV_ALIGN_CENTER, 0, 0);
    _wheel_obj->setSize(kPanelWidth, kPanelHeight);
    _wheel_obj->setRadius(0);
    _wheel_obj->setBorderWidth(0);
    _wheel_obj->setPaddingAll(0);
    _wheel_obj->setBgOpa(LV_OPA_TRANSP);
    _wheel_obj->removeFlag(LV_OBJ_FLAG_CLICKABLE);
    _wheel_obj->removeFlag(LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(_wheel_obj->get(), wheelDrawEventCb, LV_EVENT_ALL, this);
    lv_obj_refresh_ext_draw_size(_wheel_obj->get());

    _sector_labels.clear();
    _sector_labels.reserve(_option_count);
    const auto colorOrder = buildColorOrder(_option_count);

    for (int i = 0; i < _option_count; ++i)
    {
        auto label = std::make_unique<Label>(_panel->get());
        label->setSize(kLabelWidth, kLabelHeight);
        label->align(LV_ALIGN_CENTER, 0, kLabelRadius);
        label->setText(std::to_string(i + 1));
        label->setTextFont(&lv_font_montserrat_24);
        label->setTextAlign(LV_TEXT_ALIGN_CENTER);

        uint32_t sectorColor = kSectorColors[colorOrder[i]];
        label->setTextColor(lv_color_hex(uitk::color::blend_in_difference(sectorColor, kLabelFg).toHex()));
        label->setTransformPivot(kLabelWidth / 2, -kLabelRadius + kLabelHeight / 2);

        int centerDeg = 270 + (360 * (2 * i + 1)) / (2 * _option_count);
        label->setRotation((centerDeg - 90) * 10);
        label->moveForeground();
        _sector_labels.push_back(std::move(label));
    }

    _pointer_image = std::make_unique<Image>(_panel->get());
    _pointer_image->setSrc(&lucky_wheel_pointer);
    const int pointerPivotX = lucky_wheel_pointer.header.w / 2;
    const int pointerPivotY = lucky_wheel_pointer.header.h - kPointerAnchorUp;
    _pointer_image->setPos(kPanelCenterX - pointerPivotX, kPanelCenterY - pointerPivotY);
    _pointer_image->setPivot(pointerPivotX, pointerPivotY);
    _pointer_image->setRotation(0);
    lv_image_set_scale(_pointer_image->get(), kPointerScale);
    _pointer_image->moveForeground();

    _panel->onClick().connect([this]()
                              { startSpin(SpinDirection::Random); });
}

void WheelView::update()
{
    if (!_is_spinning)
    {
        return;
    }

    uint32_t now = static_cast<uint32_t>(millis());
    uint32_t elapsed = now - _spin_start_ms;
    float t = _spin_duration_ms == 0 ? 1.0f : static_cast<float>(elapsed) / static_cast<float>(_spin_duration_ms);
    if (t >= 1.0f)
    {
        applyPointerRotation(_spin_target_deg);
        _is_spinning = false;
        return;
    }

    float eased = ease::ease_out_cubic(std::clamp(t, 0.0f, 1.0f));
    float degrees = _spin_start_deg + (_spin_target_deg - _spin_start_deg) * eased;
    applyPointerRotation(degrees);
}

bool WheelView::startSpin(SpinDirection direction)
{
    if (_is_spinning || _pointer_image == nullptr || _option_count < 2)
    {
        return false;
    }

    auto& random = Random::getInstance();
    if (direction == SpinDirection::Random)
    {
        direction = random.getInt(0, 1) == 0 ? SpinDirection::Counterclockwise : SpinDirection::Clockwise;
    }

    int targetSector = random.getInt(0, _option_count - 1);
    int fullTurns = random.getInt(kSpinMinTurns, kSpinMaxTurns);
    float sectorWidthDeg = 360.0f / static_cast<float>(_option_count);
    float sectorCenterDeg = 360.0f * static_cast<float>(2 * targetSector + 1) / static_cast<float>(2 * _option_count);
    float edgeSafeDeg = std::min(kSectorEdgeSafeMaxDeg, sectorWidthDeg * kSectorEdgeSafeRatio);
    float halfRandomRangeDeg = std::max(0.0f, sectorWidthDeg * 0.5f - edgeSafeDeg);
    float sectorOffsetDeg = random.getFloat(-halfRandomRangeDeg, halfRandomRangeDeg);

    _spin_start_deg = _pointer_rotation_deg;
    _spin_target_deg = computeSignedSpinTarget(_spin_start_deg, sectorCenterDeg + sectorOffsetDeg, fullTurns, direction);
    _spin_start_ms = static_cast<uint32_t>(millis());
    _spin_duration_ms = static_cast<uint32_t>(
        random.getInt(static_cast<int>(kSpinMinDurationMs), static_cast<int>(kSpinMaxDurationMs)));
    _is_spinning = true;
    return true;
}

void WheelView::applyPointerRotation(float degrees)
{
    _pointer_rotation_deg = degrees;
    if (_pointer_image == nullptr)
    {
        return;
    }

    _pointer_image->setRotation(static_cast<int32_t>(std::lround(normalizeDegrees(degrees) * 10.0f)));
}

namespace
{

    void wheelDrawEventCb(lv_event_t* e)
    {
        auto* view = static_cast<WheelView*>(lv_event_get_user_data(e));
        if (view == nullptr)
        {
            return;
        }

        lv_event_code_t code = lv_event_get_code(e);
        if (code == LV_EVENT_REFR_EXT_DRAW_SIZE)
        {
            lv_event_set_ext_draw_size(e, kWheelOuterRadius);
            return;
        }

        if (code == LV_EVENT_COVER_CHECK)
        {
            lv_event_set_cover_res(e, LV_COVER_RES_COVER);
            return;
        }

        if (code != LV_EVENT_DRAW_MAIN_BEGIN)
        {
            return;
        }

        lv_obj_t* obj = lv_event_get_target_obj(e);
        lv_layer_t* layer = lv_event_get_layer(e);
        lv_area_t coords;
        lv_obj_get_coords(obj, &coords);

        lv_point_t center = {
            static_cast<lv_coord_t>(coords.x1 + lv_obj_get_width(obj) / 2),
            static_cast<lv_coord_t>(coords.y1 + lv_obj_get_height(obj) / 2),
        };

        lv_draw_arc_dsc_t drawDsc;
        lv_draw_arc_dsc_init(&drawDsc);
        drawDsc.opa = LV_OPA_COVER;
        drawDsc.center = center;
        drawDsc.radius = kWheelOuterRadius;
        drawDsc.width = kWheelWidth;
        drawDsc.rounded = 0;

        const int optionCount = view->optionCount();
        const auto colorOrder = buildColorOrder(optionCount);

        for (int i = 0; i < optionCount; ++i)
        {
            int startDeg = 270 + (360 * i) / optionCount;
            int endDeg = 270 + (360 * (i + 1)) / optionCount;
            uint32_t sectorColor = kSectorColors[colorOrder[i]];
            drawDsc.color = lv_color_hex(sectorColor);
            drawDsc.start_angle = startDeg;
            drawDsc.end_angle = endDeg;
            lv_draw_arc(layer, &drawDsc);
        }
    }

} // namespace
