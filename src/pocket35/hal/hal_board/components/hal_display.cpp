#include <Arduino.h>
#include "../hal_config.h"
#include "../hal_board.h"

class LGFX_Pocket35 : public lgfx::LGFX_Device
{
    lgfx::Panel_ST7796 _panel;
    lgfx::Bus_SPI _bus;
    lgfx::Light_PWM _light;

public:
    LGFX_Pocket35()
    {
        {
            auto cfg = _bus.config();
            cfg.spi_host = SPI2_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = 80000000;
            cfg.freq_read = 16000000;
            cfg.spi_3wire = true;
            cfg.use_lock = true;
            cfg.dma_channel = SPI_DMA_CH_AUTO;
            cfg.pin_sclk = HAL_PIN_LCD_SCLK;
            cfg.pin_mosi = HAL_PIN_LCD_MOSI;
            cfg.pin_miso = HAL_PIN_LCD_MISO;
            cfg.pin_dc = HAL_PIN_LCD_DC;
            _bus.config(cfg);
            _panel.setBus(&_bus);
        }
        {
            auto cfg = _panel.config();
            cfg.pin_cs = HAL_PIN_LCD_CS;
            cfg.pin_rst = HAL_PIN_LCD_RST;
            cfg.pin_busy = -1;
            cfg.panel_width = HAL_PANEL_WIDTH;
            cfg.panel_height = HAL_PANEL_HEIGHT;
            cfg.offset_x = 0;
            cfg.offset_y = 0;
            cfg.offset_rotation = 0;
            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits = 1;
            cfg.readable = false;
            cfg.invert = true;
            cfg.rgb_order = false;
            cfg.dlen_16bit = false;
            cfg.bus_shared = false;
            _panel.config(cfg);
        }
        {
            auto cfg = _light.config();
            cfg.pin_bl = HAL_PIN_LCD_BL;
            cfg.invert = false;
            cfg.freq = 44100;
            cfg.pwm_channel = 7;
            _light.config(cfg);
            _panel.setLight(&_light);
        }

        setPanel(&_panel);
    }
};

void HAL_Board::_disp_init()
{
    _display = new LGFX_Pocket35;
    _display->init();
    _display->setRotation(HAL_LCD_ROTATION);

    _canvas = new LGFX_SpriteFx(_display);
    _canvas->setPsram(true);
    _canvas->setColorDepth(lgfx::color_depth_t::rgb565_2Byte);
    if (_canvas->createSprite(_display->width(), _display->height()) == nullptr)
    {
        popFatalError("canvas alloc failed");
    }

    _canvas->setTextWrap(false);
    _canvas->fillScreen(TFT_BLACK);

    _disp_logo();
    _display->setBrightness(255);
}

void HAL_Board::_disp_logo()
{
    _canvas->fillScreen(TFT_BLACK);
    _canvas->setTextColor(TFT_ORANGE, TFT_BLACK);
    _canvas->setTextSize(4);
    _canvas->drawCenterString("POCKET35", _canvas->width() / 2, _canvas->height() / 2 - 30);
    _canvas->setTextColor(TFT_DARKGREY, TFT_BLACK);
    _canvas->setTextSize(1);
    _canvas->drawCenterString("kt6368a ble bridge", _canvas->width() / 2, _canvas->height() / 2 + 20);
    _canvas->drawCenterString(__DATE__ " " __TIME__, _canvas->width() / 2, _canvas->height() / 2 + 40);
    _canvas->pushSprite(0, 0);
}
