#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>

class Bridge
{
public:
    static constexpr size_t kBufferBytes = 4096;
    static constexpr size_t kHexLineBytes = 48;

    void begin();
    void onReceived(const uint8_t* data, size_t length);
    void poll();
    bool send(const uint8_t* data, size_t length);
    bool sendText(const std::string& text);

    inline bool isEchoEnabled() const { return echoEnabled; }
    inline void setEchoEnabled(bool enabled) { echoEnabled = enabled; }

    inline const std::string& lastHex() const { return lastHexLine; }
    inline size_t available() const { return writeIndex.load() - readIndex.load(); }

private:
    std::atomic<uint32_t> writeIndex{0};
    std::atomic<uint32_t> readIndex{0};
    uint8_t buffer[kBufferBytes] = {};
    std::string lastHexLine;
    bool echoEnabled = false;
    uint8_t chunk[kHexLineBytes] = {};

    void append(const uint8_t* data, size_t length);
    size_t read(uint8_t* out, size_t maxLength);
    static std::string toHex(const uint8_t* data, size_t length);
};
