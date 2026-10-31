#pragma once
#include <mooncake.h>
#include "../assets/icons/icons.h"

namespace MOONCAKE::APPS
{
    class AppClock : public mooncake::AppAbility
    {
    private:
        struct TouchState_t
        {
            bool wasPressed = false;
            int pressedButton = -1;
        };

        TouchState_t touchState;
        uint32_t lastRenderMs = 0;
        bool syncing = false;

        void render();
        void handleTouch();
        bool isInsideButton(int index, int16_t x, int16_t y) const;

    public:
        void onCreate() override;
        void onOpen() override;
        void onRunning() override;
        void onSleeping() override;
        void onClose() override;
    };
} // namespace MOONCAKE::APPS
