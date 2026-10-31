#pragma once
#include <cstddef>
#include <cstdint>

namespace KT6368A
{
    constexpr const char* kServiceUuid = "0000fff0-0000-1000-8000-00805f9b34fb";

    constexpr const char* kDataCharUuid = "0000fff1-0000-1000-8000-00805f9b34fb";
    constexpr const char* kNameCharUuid = "0000fff2-0000-1000-8000-00805f9b34fb";
    constexpr const char* kSpareCharUuid = "0000fff3-0000-1000-8000-00805f9b34fb";

    constexpr const char* kNamePrefix = "miniMax-";

    constexpr uint16_t kPreferredMtu = 517;
    constexpr size_t kMaxPacketBytes = 1024;
    constexpr uint32_t kScanDurationMs = 0;
    constexpr uint32_t kReconnectDelayMs = 3000;
} // namespace KT6368A
