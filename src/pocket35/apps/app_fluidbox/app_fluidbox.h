#pragma once
#include <mooncake.h>
#include <memory>
#include "FluidBox.h"

namespace MOONCAKE::APPS
{
    class AppFluidBox : public mooncake::AppAbility
    {
    private:
        std::unique_ptr<FluidBox> fluid;
        bool fluidReady = false;
        int64_t nextImuMicros = 0;
        int64_t nextSimMicros = 0;
        int64_t nextRenderMicros = 0;
        int64_t lastImuMicros = 0;
        bool touchDown = false;

        void updateImu(int64_t nowMicros);
        void updateTouch();
        void renderFrame();

    public:
        void onCreate() override;
        void onOpen() override;
        void onRunning() override;
        void onSleeping() override;
        void onClose() override;
    };
} // namespace MOONCAKE::APPS
