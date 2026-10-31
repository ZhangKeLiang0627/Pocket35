#pragma once
#include <mooncake.h>
#include <mooncake_templates.h>
#include <smooth_ui_toolkit.h>
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
            SmoothUIToolKit::SmoothRGB selectorColor;
            SmoothUIToolKit::Transition2D panelTransition;
            std::function<void(int)> openCallback = nullptr;

            int lastSelectedIndex = -1;
            int screenWidth = 480;
            int screenHeight = 320;

            bool wasTouched = false;
            int16_t touchStartX = 0;
            bool touchMoved = false;
        };
        Data_t _data;

        void renderSelector();
        void renderOptions();
        void renderPanel();
        void renderStatusBar();
        void updatePanelAnim();
        void handleTouch();

        void _update_camera_keyframe() override;
        void onReadInput() override;
        void onRender() override;
        void onUpdate(const SmoothUIToolKit::TimeSize_t& currentTime) override;
        void onGoLast() override;
        void onGoNext() override;
        void onClick() override;
        void onOpenEnd() override;

    public:
        void init(int width, int height);
        void addAppOption(const AppOptionProps_t& props);
        void setOpenCallback(std::function<void(int)> callback) { _data.openCallback = callback; }
    };
} // namespace VIEW

namespace MOONCAKE::APPS
{
    class AppLauncher : public mooncake::templates::AppLauncherBase
    {
    private:
        VIEW::LauncherView* launcherView = nullptr;

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
