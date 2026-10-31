#include "view.h"

using namespace view;
using namespace uitk::lvgl_cpp;

namespace
{

    constexpr int kPanelWidth = 480;
    constexpr int kPanelHeight = 320;
    constexpr int kSelectorWidth = 220;
    constexpr int kSelectorHeight = 150;
    constexpr int kOkWidth = 180;
    constexpr int kOkHeight = 52;

    constexpr uint32_t kBgColor = 0x202226;
    constexpr uint32_t kSelectorColor = 0x2C3036;
    constexpr uint32_t kSelectorSelectedColor = 0x3D434C;
    constexpr uint32_t kTitleColor = 0xFFFFFF;
    constexpr uint32_t kSelectorTextColor = 0xFFFFFF;
    constexpr uint32_t kOkColor = 0x4AD78C;
    constexpr uint32_t kOkTextColor = 0x0F5831;

} // namespace

void SelectionView::init(lv_obj_t* parent)
{
    _panel = std::make_unique<Container>(parent);
    _panel->align(LV_ALIGN_CENTER, 0, 0);
    _panel->setSize(kPanelWidth, kPanelHeight);
    _panel->setRadius(0);
    _panel->setBorderWidth(0);
    _panel->setPaddingAll(0);
    _panel->setBgColor(lv_color_hex(kBgColor));
    _panel->setBgOpa(LV_OPA_COVER);
    _panel->removeFlag(LV_OBJ_FLAG_SCROLLABLE);

    _title_label = std::make_unique<Label>(_panel->get());
    _title_label->align(LV_ALIGN_CENTER, 0, -108);
    _title_label->setText("Number of Options");
    _title_label->setTextFont(&lv_font_montserrat_24);
    _title_label->setTextColor(lv_color_hex(kTitleColor));

    _selector = std::make_unique<Roller>(_panel->get());
    _selector->align(LV_ALIGN_CENTER, 0, -8);
    _selector->setSize(kSelectorWidth, kSelectorHeight);
    _selector->setOptions("2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14\n15\n16\n17\n18", LV_ROLLER_MODE_NORMAL);
    _selector->setVisibleRowCount(3);
    _selector->setSelected(0, LV_ANIM_OFF);
    lv_obj_set_style_radius(_selector->get(), 24, LV_PART_MAIN);
    lv_obj_set_style_bg_color(_selector->get(), lv_color_hex(kSelectorColor), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(_selector->get(), LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(_selector->get(), 0, LV_PART_MAIN);
    lv_obj_set_style_text_font(_selector->get(), &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(_selector->get(), lv_color_hex(kSelectorTextColor), LV_PART_MAIN);
    lv_obj_set_style_text_align(_selector->get(), LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(_selector->get(), 12, LV_PART_MAIN);
    lv_obj_set_style_bg_color(_selector->get(), lv_color_hex(kSelectorSelectedColor), LV_PART_SELECTED);
    lv_obj_set_style_bg_opa(_selector->get(), LV_OPA_COVER, LV_PART_SELECTED);
    lv_obj_set_style_radius(_selector->get(), 14, LV_PART_SELECTED);
    lv_obj_set_style_text_font(_selector->get(), &lv_font_montserrat_24, LV_PART_SELECTED);
    lv_obj_set_style_text_color(_selector->get(), lv_color_hex(kSelectorTextColor), LV_PART_SELECTED);
    lv_obj_set_style_text_align(_selector->get(), LV_TEXT_ALIGN_CENTER, LV_PART_SELECTED);
    lv_obj_set_style_border_width(_selector->get(), 0, LV_PART_SELECTED);

    _ok_button = std::make_unique<Button>(_panel->get());
    _ok_button->align(LV_ALIGN_CENTER, 0, 112);
    _ok_button->setSize(kOkWidth, kOkHeight);
    _ok_button->setRadius(kOkHeight / 2);
    _ok_button->setBorderWidth(0);
    _ok_button->setShadowWidth(0);
    _ok_button->setBgColor(lv_color_hex(kOkColor));
    _ok_button->label().setText("OK");
    _ok_button->label().setTextFont(&lv_font_montserrat_24);
    _ok_button->label().setTextColor(lv_color_hex(kOkTextColor));
    _ok_button->label().align(LV_ALIGN_CENTER, 0, 0);
    _ok_button->onClick().connect([this]()
                                  {
        _confirmed_option_count = selectedOptionCount();
        _is_confirmed = true; });
}

int SelectionView::selectedOptionCount() const
{
    if (_selector == nullptr)
    {
        return 2;
    }

    return static_cast<int>(_selector->getSelected()) + 2;
}
