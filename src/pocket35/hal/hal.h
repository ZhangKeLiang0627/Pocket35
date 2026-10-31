#pragma once
#ifndef ESP_PLATFORM
#define LGFX_USE_V1
#include <LGFX_AUTODETECT.hpp>
#include <LovyanGFX.hpp>
#include "lgfx/v1/LGFXBase.hpp"
#include "lgfx/v1/LGFX_Sprite.hpp"
#include "lgfx/v1_autodetect/LGFX_AutoDetect_sdl.hpp"
#else
#include <LovyanGFX.hpp>
#endif
#include "lgfx_fx/lgfx_fx.h"
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <functional>
#include <string>

namespace TOUCH
{
    struct Point_t
    {
        uint16_t x = 0;
        uint16_t y = 0;
        bool isPressed = false;
    };
} // namespace TOUCH

namespace BLE
{
    enum class State
    {
        idle = 0,
        scanning,
        connecting,
        connected,
    };

    struct Peer_t
    {
        std::string name;
        std::string address;
        int8_t rssi = 0;
    };

    struct Stats_t
    {
        uint32_t rxPackets = 0;
        uint32_t rxBytes = 0;
        uint32_t txPackets = 0;
        uint32_t txBytes = 0;
        uint16_t mtu = 23;
    };
} // namespace BLE

namespace IMU
{
    struct Data_t
    {
        float accelX = 0.0f;
        float accelY = 0.0f;
        float accelZ = 0.0f;
        float gyroX = 0.0f;
        float gyroY = 0.0f;
        float gyroZ = 0.0f;
        float roll = 0.0f;
        float pitch = 0.0f;
        float yaw = 0.0f;
        float temperature = 0.0f;
        bool present = false;
    };
} // namespace IMU

namespace RTC
{
    struct Time_t
    {
        int year = 2026;
        int month = 1;
        int day = 1;
        int hour = 0;
        int minute = 0;
        int second = 0;
        bool valid = false;
    };
} // namespace RTC

namespace PMIC
{
    struct Battery_t
    {
        bool present = false;
        bool charging = false;
        bool chargeDone = false;
        bool vbus = false;
        float voltage = 0.0f;
        float percentage = 0.0f;
        float dieTemp = 0.0f;
        bool valid = false;
    };
} // namespace PMIC

namespace WIFI
{
    enum class State
    {
        idle = 0,
        configAp,
        connecting,
        connected,
    };

    struct Info_t
    {
        State state = State::idle;
        std::string ssid;
        std::string ip;
        bool hasCredential = false;
    };
} // namespace WIFI

class HAL
{
private:
    static HAL* _hal;

public:
    static HAL* Get();
    static bool Check();
    static bool Inject(HAL* hal);
    static void Destroy();

public:
    HAL() : _display(nullptr), _canvas(nullptr) {}
    virtual ~HAL() {}
    virtual std::string type() { return "Base"; }
    virtual void init() {}

protected:
    LGFX_Device* _display;
    LGFX_SpriteFx* _canvas;
    time_t _time_buffer = 0;
    TOUCH::Point_t _touch;
    BLE::State _ble_state = BLE::State::idle;
    BLE::Peer_t _ble_peer;
    BLE::Stats_t _ble_stats;
    std::function<void(const uint8_t*, size_t)> _ble_rx_callback;
    IMU::Data_t _imu;
    RTC::Time_t _rtc_time;
    PMIC::Battery_t _battery;
    WIFI::Info_t _wifi;

public:
    static LGFX_Device* GetDisplay() { return Get()->_display; }
    static LGFX_SpriteFx* GetCanvas() { return Get()->_canvas; }

    static void Delay(unsigned long milliseconds) { Get()->delay(milliseconds); }
    virtual void delay(unsigned long milliseconds);

    static unsigned long Millis() { return Get()->millis(); }
    virtual unsigned long millis();

    static void Reboot() { Get()->reboot(); }
    virtual void reboot() {}

    static void PowerOff() { Get()->powerOff(); }
    virtual void powerOff() {}

    static tm* GetLocalTime() { return Get()->getLocalTime(); }
    virtual tm* getLocalTime();

    static void CanvasUpdate() { Get()->canvasUpdate(); }
    virtual void canvasUpdate();

    static void RenderFpsPanel() { Get()->renderFpsPanel(); }
    virtual void renderFpsPanel();

    static void PopFatalError(std::string msg) { Get()->popFatalError(msg); }
    virtual void popFatalError(std::string msg);

    static void LoadLauncherFont24() { Get()->loadLauncherFont24(); }
    virtual void loadLauncherFont24();

    static void UpdateTouch() { Get()->updateTouch(); }
    virtual void updateTouch() {}

    static TOUCH::Point_t& GetTouchPoint() { return Get()->_touch; }

    static void BleInit() { Get()->bleInit(); }
    virtual void bleInit() {}

    static void BleStartScan() { Get()->bleStartScan(); }
    virtual void bleStartScan() {}

    static void BleStopScan() { Get()->bleStopScan(); }
    virtual void bleStopScan() {}

    static void BleConnect() { Get()->bleConnect(); }
    virtual void bleConnect() {}

    static void BleDisconnect() { Get()->bleDisconnect(); }
    virtual void bleDisconnect() {}

    static bool BleIsConnected() { return Get()->bleIsConnected(); }
    virtual bool bleIsConnected() { return false; }

    static void BleUpdate() { Get()->bleUpdate(); }
    virtual void bleUpdate() {}

    static bool BleSend(const uint8_t* data, size_t length) { return Get()->bleSend(data, length); }
    virtual bool bleSend(const uint8_t* data, size_t length) { return false; }

    static BLE::State GetBleState() { return Get()->_ble_state; }
    static BLE::Peer_t& GetBlePeer() { return Get()->_ble_peer; }
    static BLE::Stats_t& GetBleStats() { return Get()->_ble_stats; }

    using BleRxCallback = std::function<void(const uint8_t*, size_t)>;
    static void BleOnReceive(BleRxCallback callback) { Get()->_ble_rx_callback = callback; }

    static void BleReportState(BLE::State state) { Get()->_ble_state = state; }

    static void BleReportReceived(const uint8_t* data, size_t length)
    {
        Get()->_ble_stats.rxPackets++;
        Get()->_ble_stats.rxBytes += length;
        if (Get()->_ble_rx_callback)
        {
            Get()->_ble_rx_callback(data, length);
        }
    }

    static void ImuUpdate() { Get()->imuUpdate(); }
    virtual void imuUpdate() {}
    static IMU::Data_t& GetImuData() { return Get()->_imu; }

    static void RtcUpdate() { Get()->rtcUpdate(); }
    virtual void rtcUpdate() {}
    static RTC::Time_t& GetRtcTime() { return Get()->_rtc_time; }
    static void RtcSetTime(const RTC::Time_t& t) { Get()->rtcSetTime(t); }
    virtual void rtcSetTime(const RTC::Time_t& t) {}

    static void PmicUpdate() { Get()->pmicUpdate(); }
    virtual void pmicUpdate() {}
    static PMIC::Battery_t& GetBattery() { return Get()->_battery; }

    static void WifiUpdate() { Get()->wifiUpdate(); }
    virtual void wifiUpdate() {}
    static WIFI::Info_t& GetWifiInfo() { return Get()->_wifi; }
    static void WifiStartConfig() { Get()->wifiStartConfig(); }
    virtual void wifiStartConfig() {}

protected:
    void setBleState(BLE::State state) { _ble_state = state; }
};
