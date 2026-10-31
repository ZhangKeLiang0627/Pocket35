#pragma once
#include <lvgl.h>
#include <cstdint>
#include <memory>
#include <vector>
#include "lvgl/smooth_lvgl.hpp"
#include "uitk/short_namespace.hpp"

namespace view
{

    struct ClockTime
    {
        int hour = 0;
        int minute = 0;
        int second = 0;
        int year = 2026;
        int month = 1;
        int day = 1;
        int weekday = 0;
    };

    class FaceBase
    {
    public:
        virtual ~FaceBase() = default;
        virtual void create(lv_obj_t* parent) = 0;
        virtual void update(const ClockTime& now) = 0;
        virtual void destroy() = 0;
        virtual void onTap() {}
        virtual void onLongPress() {}
    };

    class ClassicFace : public FaceBase
    {
    public:
        void create(lv_obj_t* parent) override;
        void update(const ClockTime& now) override;
        void destroy() override;
        void onTap() override;

    private:
        std::unique_ptr<uitk::lvgl_cpp::Container> dial;
        std::unique_ptr<uitk::lvgl_cpp::Canvas> ticks;
        std::unique_ptr<uitk::lvgl_cpp::Label> timeLabel;
        std::unique_ptr<uitk::lvgl_cpp::Label> weekdayLabel;
        std::unique_ptr<uitk::lvgl_cpp::Container> datePanel;
        std::unique_ptr<uitk::lvgl_cpp::Label> dateLabel;
        std::unique_ptr<uitk::lvgl_cpp::Image> hourHand;
        std::unique_ptr<uitk::lvgl_cpp::Image> minuteHand;
        std::unique_ptr<uitk::lvgl_cpp::Image> secondHand;
        int displayMode = 0;
        int lastSecond = -1;

        void applyDisplayMode();
    };

    class BigNumberFace : public FaceBase
    {
    public:
        void create(lv_obj_t* parent) override;
        void update(const ClockTime& now) override;
        void destroy() override;
        void onTap() override;

    private:
        std::unique_ptr<uitk::lvgl_cpp::Image> hourTens;
        std::unique_ptr<uitk::lvgl_cpp::Image> hourOnes;
        std::unique_ptr<uitk::lvgl_cpp::Image> minuteTens;
        std::unique_ptr<uitk::lvgl_cpp::Image> minuteOnes;
        int paletteIndex = 0;
        int lastMinute = -1;

        void applyPalette();
    };

    class NumberFlowFace : public FaceBase
    {
    public:
        void create(lv_obj_t* parent) override;
        void update(const ClockTime& now) override;
        void destroy() override;
        void onTap() override;

    private:
        std::unique_ptr<uitk::lvgl_cpp::Container> background;
        std::unique_ptr<uitk::lvgl_cpp::Container> hourPanel;
        std::unique_ptr<uitk::lvgl_cpp::Container> minutePanel;
        std::unique_ptr<uitk::lvgl_cpp::Container> secondPanel;
        std::unique_ptr<uitk::lvgl_cpp::NumberFlow> hourFlow;
        std::unique_ptr<uitk::lvgl_cpp::NumberFlow> minuteFlow;
        std::unique_ptr<uitk::lvgl_cpp::NumberFlow> secondFlow;
        std::unique_ptr<uitk::lvgl_cpp::Label> dateLabel;
        int themeIndex = 0;
        int lastSecond = -1;

        void applyTheme();
    };

    class SimpleFace : public FaceBase
    {
    public:
        void create(lv_obj_t* parent) override;
        void update(const ClockTime& now) override;
        void destroy() override;
        void onTap() override;
        void onLongPress() override;

    private:
        std::unique_ptr<uitk::lvgl_cpp::Container> background;
        std::unique_ptr<uitk::lvgl_cpp::Label> timeLabel;
        std::unique_ptr<uitk::lvgl_cpp::Label> dateLabel;
        std::unique_ptr<uitk::lvgl_cpp::Label> tipsLabel;
        std::unique_ptr<uitk::lvgl_cpp::Container> secondBall;
        uitk::AnimateValue orbitAngle;
        int themeIndex = 0;
        int lastSecond = -1;
        uint32_t tipsHideTick = 0;
        bool showSecondBall = true;
        bool showTips = true;
        bool orbitInitialized = false;
        float orbitUnwrapped = 0.0f;

        void applySecondBall();
        void setOrbitAngle(float angle);
        void applyTheme();
    };

    class FaceManager
    {
    public:
        ~FaceManager();

        void init(lv_obj_t* parent);
        void update();
        void goNext();
        void goPrevious();
        int count() const { return static_cast<int>(faces.size()); }
        int currentFace() const { return currentIndex; }

    private:
        std::unique_ptr<uitk::lvgl_cpp::Container> panel;
        std::unique_ptr<uitk::lvgl_cpp::Container> pageDots;
        std::vector<std::unique_ptr<uitk::lvgl_cpp::Container>> dotItems;
        std::vector<std::unique_ptr<FaceBase>> faces;
        int nextIndex = -1;
        int currentIndex = -1;

        bool pressing = false;
        int16_t pressX = 0;
        int16_t pressY = 0;
        uint32_t pressStartMs = 0;
        int16_t lastX = 0;
        int16_t lastY = 0;

        ClockTime clock;
        uint32_t lastClockMs = 0;
        bool clockValid = false;

        void updateClock();
        void updateGesture();
        void applyPageDots();
    };

} // namespace view
