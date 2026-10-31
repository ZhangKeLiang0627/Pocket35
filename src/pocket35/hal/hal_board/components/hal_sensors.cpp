#include <Wire.h>
#include <math.h>
#include <time.h>
#include <ImuDrv.hpp>
#include <RtcDrv.hpp>
#include <PmicXPowers.hpp>
#include "../hal_config.h"
#include "../hal_board.h"

namespace
{
    SensorQMI8658 imuDevice;
    SensorPCF85063 rtcDevice;
    PmicAXP2101 pmicDevice;

    uint32_t lastImuTick = 0;
    constexpr float kYawDeadbandDeg = 0.6f;
} // namespace

void HAL_Board::_sensor_init()
{
    printf("i2c scan:");
    for (uint8_t addr = 1; addr < 127; addr++)
    {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0)
        {
            printf(" 0x%02X", addr);
        }
    }
    printf("\n");

    const uint8_t kImuAddresses[] = {QMI8658_L_SLAVE_ADDRESS, QMI8658_H_SLAVE_ADDRESS};
    bool imuReady = false;
    for (uint8_t addr : kImuAddresses)
    {
        if (imuDevice.begin(Wire, addr, HAL_PIN_I2C_SDA, HAL_PIN_I2C_SCL))
        {
            imuReady = true;
            break;
        }
    }

    if (imuReady)
    {
        imuDevice.configAccel(AccelFullScaleRange::FS_8G, 1000.0f, SensorQMI8658::LpfMode::MODE_0);
        imuDevice.configGyro(GyroFullScaleRange::FS_1000_DPS, 1000.0f, SensorQMI8658::LpfMode::MODE_0);
        imuDevice.enableAccel();
        imuDevice.enableGyro();
        _imu.present = true;
        printf("qmi8658 imu ready\n");
    }
    else
    {
        printf("qmi8658 imu not found\n");
    }

    if (rtcDevice.begin(Wire, HAL_PIN_I2C_SDA, HAL_PIN_I2C_SCL))
    {
        printf("pcf85063 rtc ready\n");
    }
    else
    {
        printf("pcf85063 rtc not found\n");
    }

    if (pmicDevice.begin(Wire, AXP2101_SLAVE_ADDRESS, HAL_PIN_I2C_SDA, HAL_PIN_I2C_SCL))
    {
        pmicDevice.enableModule(PmicAXP2101::Module::GENERAL_ADC, true);
        auto& adc = pmicDevice.adc();
        adc.enableChannels(PmicAdcBase::Channel::BAT_VOLTAGE | PmicAdcBase::Channel::BAT_PERCENTAGE | PmicAdcBase::Channel::DIE_TEMPERATURE);
        _battery.valid = true;
        printf("axp2101 pmic ready\n");
    }
    else
    {
        printf("axp2101 pmic not found\n");
    }
}

void HAL_Board::imuUpdate()
{
    if (!_imu.present)
    {
        return;
    }

    uint32_t now = millis();
    float dt = (now - lastImuTick) / 1000.0f;
    lastImuTick = now;
    if (dt <= 0.0f || dt > 0.2f)
    {
        dt = 0.05f;
    }

    if (imuDevice.isDataReady(static_cast<uint8_t>(ImuBase::DataReadyMask::ACCEL)))
    {
        AccelerometerData accel;
        if (imuDevice.readAccel(accel))
        {
            _imu.accelX = accel.mps2.x;
            _imu.accelY = accel.mps2.y;
            _imu.accelZ = accel.mps2.z;
        }
    }

    if (imuDevice.isDataReady(static_cast<uint8_t>(ImuBase::DataReadyMask::GYRO)))
    {
        GyroscopeData gyro;
        if (imuDevice.readGyro(gyro))
        {
            _imu.gyroX = gyro.dps.x;
            _imu.gyroY = gyro.dps.y;
            _imu.gyroZ = gyro.dps.z;
        }
    }

    _imu.temperature = imuDevice.getTemperature();

    _imu.roll = atan2f(_imu.accelY, _imu.accelZ) * 180.0f / (float)M_PI;
    _imu.pitch = atan2f(-_imu.accelX, sqrtf(_imu.accelY * _imu.accelY + _imu.accelZ * _imu.accelZ)) * 180.0f / (float)M_PI;

    if (fabsf(_imu.gyroZ) > kYawDeadbandDeg)
    {
        _imu.yaw += _imu.gyroZ * dt;
    }
}

void HAL_Board::rtcUpdate()
{
    static uint32_t lastUpdate = 0;
    uint32_t now = millis();
    if (now - lastUpdate < 1000)
    {
        return;
    }
    lastUpdate = now;

    RTC_DateTime dt = rtcDevice.getDateTime();
    _rtc_time.year = dt.getYear();
    _rtc_time.month = dt.getMonth();
    _rtc_time.day = dt.getDay();
    _rtc_time.hour = dt.getHour();
    _rtc_time.minute = dt.getMinute();
    _rtc_time.second = dt.getSecond();
    _rtc_time.valid = true;
}

void HAL_Board::rtcSetTime(const RTC::Time_t& t)
{
    rtcDevice.setDateTime(t.year, t.month, t.day, t.hour, t.minute, t.second);
}

void HAL_Board::ntpSync()
{
    configTime(8 * 3600, 0, "ntp.aliyun.com", "pool.ntp.org");

    struct tm timeinfo;
    if (!::getLocalTime(&timeinfo, 8000))
    {
        printf("ntp: sync failed\n");
        return;
    }

    RTC::Time_t t;
    t.year = timeinfo.tm_year + 1900;
    t.month = timeinfo.tm_mon + 1;
    t.day = timeinfo.tm_mday;
    t.hour = timeinfo.tm_hour;
    t.minute = timeinfo.tm_min;
    t.second = timeinfo.tm_sec;
    t.valid = true;
    rtcSetTime(t);

    _rtc_sync.synced = true;
    _rtc_sync.year = t.year;
    _rtc_sync.month = t.month;
    _rtc_sync.day = t.day;
    _rtc_sync.hour = t.hour;
    _rtc_sync.minute = t.minute;

    printf("ntp: synced %04d-%02d-%02d %02d:%02d:%02d\n", t.year, t.month, t.day, t.hour, t.minute, t.second);
}

void HAL_Board::pmicUpdate()
{
    if (!_battery.valid)
    {
        return;
    }

    static uint32_t lastUpdate = 0;
    uint32_t now = millis();
    if (now - lastUpdate < 2000)
    {
        return;
    }
    lastUpdate = now;

    _battery.present = pmicDevice.isBatteryConnect();
    _battery.vbus = pmicDevice.isVbusGood();

    auto& adc = pmicDevice.adc();
    float value = 0.0f;
    if (adc.read(PmicAdcBase::Channel::BAT_VOLTAGE, value))
    {
        _battery.voltage = value;
    }
    if (adc.read(PmicAdcBase::Channel::BAT_PERCENTAGE, value))
    {
        _battery.percentage = value;
    }
    if (adc.read(PmicAdcBase::Channel::DIE_TEMPERATURE, value))
    {
        _battery.dieTemp = value;
    }

    auto* charger = pmicDevice.getCharger();
    if (charger != nullptr)
    {
        auto status = charger->getStatus();
        _battery.charging = status.charging;
        _battery.chargeDone = status.chargeDone;
    }
}
