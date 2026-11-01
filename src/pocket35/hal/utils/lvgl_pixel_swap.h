#pragma once

#include <cstdint>

namespace PIXEL
{
    inline void swapRgb565Bytes(uint8_t* _buffer, uint32_t _count)
    {
        if (((uintptr_t)_buffer & 3) == 0)
        {
            uint32_t* quad = (uint32_t*)_buffer;
            const uint32_t pairs = _count >> 1;
            for (uint32_t i = 0; i < pairs; i++)
            {
                const uint32_t v = quad[i];
                quad[i] = ((v >> 8) & 0x00FF00FF) | ((v << 8) & 0xFF00FF00);
            }
            if (_count & 1)
            {
                uint16_t* tail = (uint16_t*)_buffer + (_count - 1);
                *tail = (uint16_t)((*tail >> 8) | (*tail << 8));
            }
            return;
        }

        uint16_t* pixels = (uint16_t*)_buffer;
        for (uint32_t i = 0; i < _count; i++)
        {
            pixels[i] = (pixels[i] >> 8) | (pixels[i] << 8);
        }
    }
} // namespace PIXEL
