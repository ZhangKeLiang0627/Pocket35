// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
// https://github.com/m5stack/M5Unified/blob/master/src/utility/Button_Class.hpp
// Ported from M5StopWatch-UserDemo main/hal/utils/button/Button_Class.hpp

#ifndef __M5_BUTTON_CLASS_H__
#define __M5_BUTTON_CLASS_H__

#include <cstdint>

namespace m5 {
class Button_Class {
public:
    enum button_state_t : std::uint8_t { state_nochange, state_clicked, state_hold, state_decide_click_count };

    bool wasClicked(void) const
    {
        return _currentState == state_clicked;
    }

    bool wasHold(void) const
    {
        return _currentState == state_hold;
    }

    bool wasSingleClicked(void) const
    {
        return _currentState == state_decide_click_count && _clickCount == 1;
    }

    bool wasDoubleClicked(void) const
    {
        return _currentState == state_decide_click_count && _clickCount == 2;
    }

    bool wasDecideClickCount(void) const
    {
        return _currentState == state_decide_click_count;
    }

    std::uint8_t getClickCount(void) const
    {
        return _clickCount;
    }

    bool isHolding(void) const
    {
        return _press == 2;
    }
    bool wasChangePressed(void) const
    {
        return ((bool)_press) != ((bool)_oldPress);
    }

    bool isPressed(void) const
    {
        return _press;
    }
    bool isReleased(void) const
    {
        return !_press;
    }
    bool wasPressed(void) const
    {
        return !_oldPress && _press;
    }
    bool wasReleased(void) const
    {
        return _oldPress && !_press;
    }
    bool wasReleasedAfterHold(void) const
    {
        return !_press && _oldPress == 2;
    }
    bool wasReleaseFor(std::uint32_t ms) const
    {
        return _oldPress && !_press && _lastHoldPeriod >= ms;
    }

    bool pressedFor(std::uint32_t ms) const
    {
        return (_press && _lastMsec - _lastChange >= ms);
    }
    bool releasedFor(std::uint32_t ms) const
    {
        return (!_press && _lastMsec - _lastChange >= ms);
    }

    void setDebounceThresh(std::uint32_t msec)
    {
        _msecDebounce = msec;
    }
    void setHoldThresh(std::uint32_t msec)
    {
        _msecHold = msec;
    }

    void setRawState(std::uint32_t msec, bool press);
    void setState(std::uint32_t msec, button_state_t state);
    button_state_t getState(void) const
    {
        return _currentState;
    }
    std::uint32_t lastChange(void) const
    {
        return _lastChange;
    }

    std::uint32_t getDebounceThresh(void) const
    {
        return _msecDebounce;
    }
    std::uint32_t getHoldThresh(void) const
    {
        return _msecHold;
    }

    std::uint32_t getUpdateMsec(void) const
    {
        return _lastMsec;
    }

private:
    std::uint32_t _lastMsec       = 0;
    std::uint32_t _lastChange     = 0;
    std::uint32_t _lastRawChange  = 0;
    std::uint32_t _lastClicked    = 0;
    std::uint16_t _msecDebounce   = 10;
    std::uint16_t _msecHold       = 500;
    std::uint16_t _lastHoldPeriod = 0;
    button_state_t _currentState  = state_nochange;
    bool _raw_press               = false;
    std::uint8_t _press           = 0;
    std::uint8_t _oldPress        = 0;
    std::uint8_t _clickCount      = 0;
};

}  // namespace m5

#endif
