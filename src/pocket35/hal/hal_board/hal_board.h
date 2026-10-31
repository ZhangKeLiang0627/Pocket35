#pragma once
#include "../hal.h"

class HAL_Board : public HAL
{
private:
    void _i2c_init();
    void _io_expander_init();
    void _io_expander_write(uint8_t value);
    uint8_t _io_expander_read(uint8_t reg);
    void _io_expander_write_reg(uint8_t reg, uint8_t value);
    void _disp_init();
    void _disp_logo();
    void _touch_init();
    void _sensor_init();
    void _wifi_init();
    void _sum_up();

public:
    HAL_Board() {}
    ~HAL_Board() {}

    inline std::string type() override { return "Pocket35"; }

    void init() override;

    void reboot() override;
    void powerOff() override;

    void loadLauncherFont24() override;
    void updateTouch() override;

    void bleInit() override;
    void bleStartScan() override;
    void bleStopScan() override;
    void bleConnect() override;
    void bleDisconnect() override;
    bool bleIsConnected() override;
    bool bleSend(const uint8_t* data, size_t length) override;
    void bleUpdate() override;

    void imuUpdate() override;
    void rtcUpdate() override;
    void rtcSetTime(const RTC::Time_t& t) override;
    void ntpSync() override;
    void pmicUpdate() override;

    void audioInit() override;
    void audioOpen() override;
    void audioClose() override;
    void audioUpdate() override;
    void playBootSound() override;
    void playSfx(AUDIO::Sfx sfx) override;
    void setVolume(int percent) override;

    void wifiUpdate() override;
    void wifiStartConfig() override;

    void buttonUpdate() override;
    BUTTON::Event getButtonEvent() override;
    bool isButtonPressed() override;
    bool isButtonHolding() override;
    uint32_t getButtonPressedMs() override;
    void setButtonSfx(bool enabled) override;

    void lvglInit() override;
    void lvglUpdate() override;
    void lvglFlushNow() override;
    void lvglSuspend() override;
    void lvglResume() override;
    void canvasUpdate() override;
    void canvasPush(int16_t x, int16_t y, int16_t w, int16_t h) override;
};
