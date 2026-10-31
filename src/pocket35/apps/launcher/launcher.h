#pragma once
#include <mooncake.h>
#include <mooncake_templates.h>
#include <smooth_ui_toolkit.h>
#include <LovyanGFX.hpp>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace VIEW
{
    class LauncherView : public SmoothUIToolKit::SelectMenu::SmoothSelector
    {
    public:
        struct AppOptionProps_t
        {
            int appID = -1;
            uint32_t themeColor = 0;
            void* icon = nullptr;
            std::string name;
        };

    private:
        struct Data_t
        {
            std::vector<AppOptionProps_t> optionPropsList;
            std::vector<lgfx::LGFX_Sprite*> cardCache;
            SmoothUIToolKit::SmoothRGB selectorColor;
            SmoothUIToolKit::Transition2D panelTransition;
            std::function<void(int)> openCallback = nullptr;

            int lastSelectedIndex = -1;
            int lastPanelY = 262;
            int screenWidth = 480;
            int screenHeight = 320;

            bool wasTouched = false;
            int16_t touchStartX = 0;
            int16_t dragOffset = 0;
            bool isDragging = false;
            bool touchOnIcon = false;

            int16_t lastTouchX = 0;
            SmoothUIToolKit::TimeSize_t lastTouchTime = 0;
            float swipeVelocity = 0.0f;

            bool isDirty = true;
            bool wasAnimating = false;
            bool forceFullRedraw = true;
            char lastStatusText[32] = {0};
        };
        Data_t _data;

        void _buildCardCache(size_t index);
        void _clearRows(int y, int height);
        void _renderCardAt(size_t index, int cameraX, const SmoothUIToolKit::Vector4D_t& selectorFrame);
        void _trackSwipeVelocity(int16_t x, SmoothUIToolKit::TimeSize_t now);
        void _releaseDrag();
        int _cameraX();
        void renderSelector();
        void renderOptions();
        void renderPanel();
        void renderStatusBar();
        void updatePanelAnim();
        void handleTouch();
        bool isTouchInsideIcon(int16_t x, int16_t y);

        void _update_camera_keyframe() override;
        void onReadInput() override;
        void onRender() override;
        void onUpdate(const SmoothUIToolKit::TimeSize_t& currentTime) override;
        void onGoLast() override;
        void onGoNext() override;
        void onClick() override;
        void onOpenEnd() override;

    public:
        ~LauncherView() override;
        void init(int width, int height);
        void renderNow() { onRender(); }
        void addAppOption(const AppOptionProps_t& props);
        void setSelectedOptionIndex(int index);
        void setOpenCallback(std::function<void(int)> callback) { _data.openCallback = callback; }
    };
} // namespace VIEW

namespace MOONCAKE::APPS
{
    class AppLauncher : public mooncake::templates::AppLauncherBase
    {
    private:
        VIEW::LauncherView* launcherView = nullptr;
        int lastSelectedAppId = -1;
        bool bootSoundPlayed = false;

        void createLauncherView();
        void destroyLauncherView();

    public:
        void onLauncherCreate() override;
        void onLauncherOpen() override;
        void onLauncherRunning() override;
        void onLauncherSleeping() override;
        void onLauncherClose() override;
        void onLauncherDestroy() override;
    };
} // namespace MOONCAKE::APPS
