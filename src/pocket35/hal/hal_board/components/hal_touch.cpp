#include <Wire.h>
#include "../hal_config.h"
#include "../hal_board.h"

namespace
{
    bool readTouchRegisters(uint8_t reg, uint8_t* buffer, size_t length)
    {
        Wire.beginTransmission(HAL_I2C_ADDR_TOUCH);
        Wire.write(reg);
        if (Wire.endTransmission(false) != 0)
        {
            return false;
        }
        if (Wire.requestFrom((uint8_t)HAL_I2C_ADDR_TOUCH, (uint8_t)length) != length)
        {
            return false;
        }
        for (size_t i = 0; i < length; i++)
        {
            buffer[i] = Wire.read();
        }
        return true;
    }

    uint16_t mapTouchAxis(uint16_t value, bool isX)
    {
#if HAL_TOUCH_SWAP_XY
        const uint16_t limit = isX ? HAL_PANEL_HEIGHT : HAL_PANEL_WIDTH;
        const bool doMirror = isX ? HAL_TOUCH_MIRROR_X : HAL_TOUCH_MIRROR_Y;
#else
        const uint16_t limit = isX ? HAL_PANEL_WIDTH : HAL_PANEL_HEIGHT;
        const bool doMirror = isX ? HAL_TOUCH_MIRROR_X : HAL_TOUCH_MIRROR_Y;
#endif
        return doMirror ? limit - 1 - value : value;
    }
} // namespace

void HAL_Board::_touch_init()
{
    Wire.beginTransmission(HAL_I2C_ADDR_TOUCH);
    const bool isReady = Wire.endTransmission() == 0;

    _touch.isPressed = false;
    printf("ft6336 %s at 0x%02X\n", isReady ? "found" : "missing", HAL_I2C_ADDR_TOUCH);
}

void HAL_Board::updateTouch()
{
    uint8_t pointCount = 0;
    if (!readTouchRegisters(0x02, &pointCount, 1) || (pointCount & 0x0F) == 0)
    {
        _touch.isPressed = false;
        return;
    }

    uint8_t buffer[4];
    if (!readTouchRegisters(0x03, buffer, sizeof(buffer)))
    {
        _touch.isPressed = false;
        return;
    }

    const uint16_t rawX = ((buffer[0] & 0x0F) << 8) | buffer[1];
    const uint16_t rawY = ((buffer[2] & 0x0F) << 8) | buffer[3];

#if HAL_TOUCH_SWAP_XY
    _touch.x = mapTouchAxis(rawY, true);
    _touch.y = mapTouchAxis(rawX, false);
#else
    _touch.x = mapTouchAxis(rawX, true);
    _touch.y = mapTouchAxis(rawY, false);
#endif

    _touch.isPressed = true;
}
