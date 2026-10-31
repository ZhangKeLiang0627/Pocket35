#pragma once
#include <mooncake.h>
#include "../assets/icons/icons.h"

namespace MOONCAKE::APPS
{
    class AppTemplate : public mooncake::AppAbility
    {
    private:
        struct TouchState_t
        {
            bool wasPressed = false;
            bool insideExit = false;
        };

        TouchState_t touchState;

        void render();
        void handleTouch();
        bool isInsideExit(int16_t x, int16_t y) const;

    public:
        void onCreate() override;
        void onOpen() override;
        void onRunning() override;
        void onSleeping() override;
        void onClose() override;
    };
} // namespace MOONCAKE::APPS
