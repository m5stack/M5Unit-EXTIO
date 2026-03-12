/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  LED control example using M5UnitUnified for UnitExtIO2
*/
#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedEXTIO.h>
#include <M5Utility.h>
#include <M5HAL.hpp>
#include <bitset>

using namespace m5::unit::extio2;
using m5::unit::UnitExtIO2;

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;
m5::unit::UnitExtIO2 unit;

// Bits of the target pin (LSB:pin0 ... pin7:MSB)
// e.g.
//   0X01 Pin 0
//   0X9C Pin 2,3,4,7
//   0XFF All pins
constexpr uint8_t target_pins{0xFF};
static_assert(target_pins != 0, "Specify at least one");

uint32_t counter{};
bool can_render{};
bool small_screen{};

template <typename T>
constexpr bool is_power_of_2(const T v)
{
    static_assert(std::is_integral<T>::value, "The argument v is only an integer value");
    return (v > 0) && ((v & (v - 1)) == 0);
}

// 0 -> (W-1) -> 0 ->...
template <int W, typename T>
T round_trip(const T c)
{
    static_assert(is_power_of_2(W), "W must be power of 2");
    static_assert(std::is_integral<T>::value, "The argument c is only an integer value");
    return ((c & W) + ((c & (W - 1)) ^ (0 - ((c >> (std::bitset<32>(W - 1).count())) & 1))));
}

}  // namespace

void setup()
{
    M5.begin();
    M5.setTouchButtonHeightByRatio(100);

    // The screen shall be in landscape mode
    if (lcd.height() > lcd.width()) {
        lcd.setRotation(1);
    }

    auto board = M5.getBoard();

    // NessoN1: Arduino Wire (I2C_NUM_0) cannot be used for GROVE port.
    //   Wire is used by M5Unified In_I2C for internal devices (IOExpander etc.).
    //   Wire1 exists but is reserved for HatPort — cannot be used for GROVE.
    //   Reconfiguring Wire to GROVE pins breaks In_I2C, causing ESP_ERR_INVALID_STATE in M5.update().
    //   Solution: Use SoftwareI2C via M5HAL (bit-banging) for the GROVE port.
    // NanoC6: Wire.begin() on GROVE pins conflicts with m5::I2C_Class registered by Ex_I2C.setPort()
    //   on the same I2C_NUM_0, causing sporadic NACK errors.
    //   Solution: Use M5.Ex_I2C (m5::I2C_Class) directly instead of Arduino Wire.
    bool unit_ready{};
    if (board == m5::board_t::board_ArduinoNessoN1) {
        // NessoN1: GROVE is on port_b (GPIO 5/4), not port_a (which maps to Wire pins 8/10)
        auto pin_num_sda = M5.getPin(m5::pin_name_t::port_b_out);
        auto pin_num_scl = M5.getPin(m5::pin_name_t::port_b_in);
        M5_LOGI("getPin(M5HAL): SDA:%u SCL:%u", pin_num_sda, pin_num_scl);
        m5::hal::bus::I2CBusConfig i2c_cfg;
        i2c_cfg.pin_sda = m5::hal::gpio::getPin(pin_num_sda);
        i2c_cfg.pin_scl = m5::hal::gpio::getPin(pin_num_scl);
        auto i2c_bus    = m5::hal::bus::i2c::getBus(i2c_cfg);
        M5_LOGI("Bus:%d", i2c_bus.has_value());
        unit_ready = Units.add(unit, i2c_bus ? i2c_bus.value() : nullptr) && Units.begin();
    } else if (board == m5::board_t::board_M5NanoC6) {
        // NanoC6: Use M5.Ex_I2C (m5::I2C_Class, not Arduino Wire)
        M5_LOGI("Using M5.Ex_I2C");
        unit_ready = Units.add(unit, M5.Ex_I2C) && Units.begin();
    } else {
        auto pin_num_sda = M5.getPin(m5::pin_name_t::port_a_sda);
        auto pin_num_scl = M5.getPin(m5::pin_name_t::port_a_scl);
        M5_LOGI("getPin: SDA:%u SCL:%u", pin_num_sda, pin_num_scl);
        Wire.end();
        Wire.begin(pin_num_sda, pin_num_scl, 100 * 1000U);
        unit_ready = Units.add(unit, Wire) && Units.begin();
    }
    if (!unit_ready) {
        M5_LOGE("Failed to begin");
        lcd.fillScreen(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }

    M5_LOGI("M5UnitUnified has been begun");
    M5_LOGI("%s", Units.debugInfo().c_str());

    can_render   = (lcd.width() > 0 && lcd.height() > 0 && !lcd.isEPD());
    small_screen = (lcd.width() < 200);

    unit.writePinBitsMode(target_pins, Mode::LEDControl);

    if (can_render) {
        if (small_screen) {
            lcd.setFont(&fonts::Font0);
        } else {
            lcd.setFont(&fonts::AsciiFont8x16);
        }
        lcd.startWrite();
        lcd.fillScreen(TFT_BLACK);

        lcd.drawString(small_screen ? "LED" : "LED CONTROL", 2, 2);
        std::string s = small_screen ? "P:" : "TargetPins:";
        for (uint8_t pin = 0; pin < UnitExtIO2::NUMBER_OF_PINS; ++pin) {
            if ((1U << pin) & target_pins) {
                s += m5::utility::formatString("%u", pin);
                if (!small_screen) {
                    s += ' ';
                }
            }
        }
        int16_t font_h = small_screen ? 8 : 16;
        lcd.drawString(s.c_str(), 2, 2 + font_h + 2);
        lcd.endWrite();
    }
}

void loop()
{
    M5.update();

    uint8_t r = round_trip<128>(counter) << 1;
    uint8_t g = round_trip<64>(counter) << 2;
    uint8_t b = round_trip<256>(counter);

    if (!unit.writePinBitsLEDColor(target_pins, r, g, b)) {
        M5_LOGE("Failed to write");
    }

    if (can_render) {
        int16_t font_h = small_screen ? 8 : 16;
        int16_t text_y = 2 + (font_h + 2) * 2;
        int16_t bar_y  = text_y + font_h + 2;
        lcd.startWrite();
        lcd.setCursor(2, text_y);
        lcd.printf("RGB(%3u,%3u,%3u)", r, g, b);
        lcd.fillRect(2, bar_y, lcd.width() - 4, font_h, (uint16_t)lgfx::rgb565_t(r, g, b));
        lcd.endWrite();
    }
    M5.Log.printf("RGB(%3u,%3u,%3u)\n", r, g, b);

    ++counter;
}
