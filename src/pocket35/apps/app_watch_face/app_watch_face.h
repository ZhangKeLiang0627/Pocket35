#pragma once
#include <mooncake.h>
#include <memory>
#include "view/watch_face.h"

namespace MOONCAKE::APPS
{
    class AppWatchFace : public mooncake::AppAbility
    {
    private:
        std::unique_ptr<view::FaceManager> faceManager;

    public:
        void onCreate() override;
        void onOpen() override;
        void onRunning() override;
        void onSleeping() override;
        void onClose() override;
    };
} // namespace MOONCAKE::APPS
