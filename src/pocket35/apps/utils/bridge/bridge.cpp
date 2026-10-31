#include "bridge.h"
#include <Arduino.h>
#include <cstdio>
#include "hal/hal.h"

void Bridge::begin()
{
    writeIndex.store(0);
    readIndex.store(0);
    lastHexLine.clear();
}

void Bridge::onReceived(const uint8_t* data, size_t length) { append(data, length); }

void Bridge::append(const uint8_t* data, size_t length)
{
    const uint32_t write = writeIndex.load(std::memory_order_relaxed);

    for (size_t i = 0; i < length; i++)
    {
        const uint32_t slot = (write + i) % kBufferBytes;
        buffer[slot] = data[i];
    }

    writeIndex.store(write + length, std::memory_order_release);

    if (writeIndex.load() - readIndex.load() > kBufferBytes)
    {
        readIndex.store(writeIndex.load() - kBufferBytes, std::memory_order_release);
    }

    lastHexLine = toHex(data, length > 24 ? 24 : length);
}

size_t Bridge::read(uint8_t* out, size_t maxLength)
{
    const uint32_t write = writeIndex.load(std::memory_order_acquire);
    const uint32_t read = readIndex.load(std::memory_order_relaxed);
    const uint32_t pending = write - read;
    const size_t count = pending < maxLength ? pending : maxLength;

    for (size_t i = 0; i < count; i++)
    {
        out[i] = buffer[(read + i) % kBufferBytes];
    }

    readIndex.store(read + count, std::memory_order_release);
    return count;
}

void Bridge::poll()
{
    size_t count = 0;
    while ((count = read(chunk, sizeof(chunk))) > 0)
    {
        printf("[ble rx] %s\n", toHex(chunk, count).c_str());

        if (echoEnabled)
        {
            HAL::BleSend(chunk, count);
        }
    }
}

bool Bridge::send(const uint8_t* data, size_t length) { return HAL::BleSend(data, length); }

bool Bridge::sendText(const std::string& text)
{
    return HAL::BleSend(reinterpret_cast<const uint8_t*>(text.data()), text.size());
}

std::string Bridge::toHex(const uint8_t* data, size_t length)
{
    static const char* digits = "0123456789ABCDEF";

    std::string out;
    out.reserve(length * 3);
    for (size_t i = 0; i < length; i++)
    {
        out.push_back(digits[data[i] >> 4]);
        out.push_back(digits[data[i] & 0x0F]);
        out.push_back(' ');
    }
    if (!out.empty())
    {
        out.pop_back();
    }
    return out;
}
