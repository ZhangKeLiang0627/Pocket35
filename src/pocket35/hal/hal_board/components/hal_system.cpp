#include <Wire.h>
#include <esp_sleep.h>
#include "../hal_config.h"
#include "../hal_board.h"

namespace
{
    BUTTON::Event pendingEvent = BUTTON::Event::none;
} // namespace

void HAL_Board::_i2c_init() { Wire.begin(HAL_PIN_I2C_SDA, HAL_PIN_I2C_SCL, 400000); }

uint8_t HAL_Board::_io_expander_read(uint8_t reg)
{
    Wire.beginTransmission(HAL_I2C_ADDR_IO_EXPANDER);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0)
    {
        return 0xFF;
    }
    if (Wire.requestFrom((uint8_t)HAL_I2C_ADDR_IO_EXPANDER, (uint8_t)1) != 1)
    {
        return 0xFF;
    }
    return Wire.read();
}

void HAL_Board::_io_expander_write_reg(uint8_t reg, uint8_t value)
{
    Wire.beginTransmission(HAL_I2C_ADDR_IO_EXPANDER);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

void HAL_Board::_io_expander_write(uint8_t value)
{
    _io_expander_write_reg(0x01, value);
}

void HAL_Board::_io_expander_init()
{
    uint8_t config = _io_expander_read(0x03);
    config &= ~(1 << HAL_IO_EXPANDER_PIN_LCD_RST);
    _io_expander_write_reg(0x03, config);

    uint8_t output = _io_expander_read(0x01);
    output &= ~(1 << HAL_IO_EXPANDER_PIN_LCD_RST);
    _io_expander_write(output);
    delay(100);

    output |= (1 << HAL_IO_EXPANDER_PIN_LCD_RST);
    _io_expander_write(output);
    delay(100);
}

void HAL_Board::init()
{
    _i2c_init();
    _io_expander_init();
    _disp_init();
    _touch_init();
    lvglInit();
    _sensor_init();
    _wifi_init();
    _sum_up();
    audioInit();

    pinMode(HAL_PIN_BUTTON_BOOT, INPUT_PULLUP);
    btnBoot.setRawState(millis(), digitalRead(HAL_PIN_BUTTON_BOOT) == LOW);
}

void HAL_Board::reboot()
{
    delay(50);
    ESP.restart();
}

void HAL_Board::buttonUpdate()
{
    btnBoot.setRawState(millis(), digitalRead(HAL_PIN_BUTTON_BOOT) == LOW);

    if (btnBoot.wasPressed())
    {
        playSfx(AUDIO::Sfx::tap);
    }
    else if (btnBoot.wasReleased())
    {
        playSfx(AUDIO::Sfx::release);
    }

    if (btnBoot.wasHold())
    {
        pendingEvent = BUTTON::Event::hold;
    }
    else if (btnBoot.wasReleasedAfterHold())
    {
        pendingEvent = BUTTON::Event::holdRelease;
    }
    else if (btnBoot.wasClicked())
    {
        pendingEvent = BUTTON::Event::click;
    }
}

BUTTON::Event HAL_Board::getButtonEvent()
{
    const BUTTON::Event event = pendingEvent;
    pendingEvent = BUTTON::Event::none;
    return event;
}

bool HAL_Board::isButtonPressed() { return btnBoot.isPressed(); }

bool HAL_Board::isButtonHolding() { return btnBoot.isHolding(); }

uint32_t HAL_Board::getButtonPressedMs()
{
    return btnBoot.isPressed() ? millis() - btnBoot.lastChange() : 0;
}

void HAL_Board::setButtonSfx(bool enabled) { buttonConfig.sfxEnabled = enabled; }

void HAL_Board::powerOff()
{
    delay(50);
    esp_deep_sleep_start();
}

void HAL_Board::loadLauncherFont24()
{
    _canvas->setFont(HAL_LAUNCHER_FONT);
    _canvas->setTextSize(1);
}

void HAL_Board::_sum_up()
{
    _canvas->setTextColor(TFT_LIGHTGRAY, TFT_BLACK);
    _canvas->setTextSize(1);
    _canvas->setCursor(12, 12);
    _canvas->printf("pocket35 hal ready, %dx%d", _canvas->width(), _canvas->height());
    _canvas->pushSprite(0, 0);
    printf("pocket35 hal ready, canvas %dx%d\n", _canvas->width(), _canvas->height());
    delay(400);
}
