#pragma once
#include <mooncake.h>
#include <memory>
#include "view/view.h"

namespace MOONCAKE::APPS
{
    class AppLuckyWheel : public mooncake::AppAbility
    {
    private:
        std::unique_ptr<view::SelectionView> selectionView;
        std::unique_ptr<view::WheelView> wheelView;

    public:
        void onCreate() override;
        void onOpen() override;
        void onRunning() override;
        void onSleeping() override;
        void onClose() override;
    };
} // namespace MOONCAKE::APPS
