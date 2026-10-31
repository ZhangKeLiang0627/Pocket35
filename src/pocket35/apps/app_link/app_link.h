#pragma once
#include <mooncake.h>
#include "hal/hal.h"
#include "apps/utils/bridge/bridge.h"

namespace MOONCAKE::APPS
{
    class AppLink : public mooncake::AppAbility
    {
    private:
        struct TouchState_t
        {
            bool wasPressed = false;
            int pressedButton = -1;
        };

        Bridge bridge;
        TouchState_t touchState;
        uint32_t lastRenderMs = 0;

        void render();
        void renderHeader();
        void renderStatus();
        void renderStats();
        void renderPayload();
        void renderButtons();
        void handleTouch();
        bool isInside(int index, int16_t x, int16_t y) const;
        static const char* stateText(BLE::State state);

    public:
        void onCreate() override;
        void onOpen() override;
        void onRunning() override;
        void onSleeping() override;
        void onClose() override;
    };
} // namespace MOONCAKE::APPS
