/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Digital output example using M5UnitUnified for UnitExtIO2

  Operation:
    Click BtnA : Toggle HIGH/LOW on current pin
    Hold  BtnA : Current pin to next
*/
#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedEXTIO.h>
#include <M5Utility.h>
#include <M5HAL.hpp>

using namespace m5::unit::extio2;
using m5::unit::UnitExtIO2;

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;
m5::unit::UnitExtIO2 unit;

uint8_t state{};
uint8_t current{};
bool can_render{};
bool small_screen{};
int16_t font_h{16};

void render()
{
    if (!can_render) {
        return;
    }
    lcd.drawString(small_screen ? "DIG OUT" : "DIGITAL OUTPUT", 2, 2);

    int16_t row1 = 2 + font_h + 2;
    int16_t row2 = row1 + font_h + 2;
    lcd.setCursor(2, row1);
    if (small_screen) {
        lcd.printf("%u%u%u%u%u%u%u%u", (bool)(state & 0x01), (bool)(state & 0x02), (bool)(state & 0x04),
                   (bool)(state & 0x08), (bool)(state & 0x10), (bool)(state & 0x20), (bool)(state & 0x40),
                   (bool)(state & 0x80));
    } else {
        lcd.printf("Pin0:%u%u%u%u%u%u%u%u:Pin7", (bool)(state & 0x01), (bool)(state & 0x02), (bool)(state & 0x04),
                   (bool)(state & 0x08), (bool)(state & 0x10), (bool)(state & 0x20), (bool)(state & 0x40),
                   (bool)(state & 0x80));
    }
    lcd.setCursor(2, row2);
    lcd.printf(small_screen ? "Pin:%u" : "Current Pin:%u", current);
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

    if (can_render) {
        if (small_screen) {
            lcd.setFont(&fonts::Font0);
            font_h = 8;
        } else {
            lcd.setFont(&fonts::AsciiFont8x16);
            font_h = 16;
        }
        lcd.startWrite();
        lcd.fillScreen(TFT_BLACK);
        lcd.endWrite();
    }

    unit.writeAllMode(Mode::DigitalOutput);
    unit.writeAllDigitalOutput(false);  // All pins to LOW
    M5.Log.printf("Current Pin:%u\n", current);
}

void loop()
{
    static bool dirty{true};

    M5.update();

    if (M5.BtnA.wasClicked()) {
        // Toggle HIGH/LOW
        state ^= (1U << current);
        M5.Log.printf("Change Pin:%u to %s\n", current, (state & (1U << current)) ? "HIGH" : "LOW");
        dirty = true;
    } else if (M5.BtnA.wasHold()) {
        // Change current to next
        current = (current + 1) % UnitExtIO2::NUMBER_OF_PINS;
        M5.Log.printf("Current Pin:%u\n", current);
        dirty = true;
    }

    if (dirty) {
        if (!unit.writePinBitsDigitalOutput(0xFF /* All pins */, state)) {
            M5_LOGE("Failed to output");
        }
        if (can_render) {
            lcd.startWrite();
            render();
            lcd.endWrite();
        }
        M5.Log.printf("Output Pin0:%u%u%u%u%u%u%u%u:Pin7\n", (bool)(state & 0x01), (bool)(state & 0x02),
                      (bool)(state & 0x04), (bool)(state & 0x08), (bool)(state & 0x10), (bool)(state & 0x20),
                      (bool)(state & 0x40), (bool)(state & 0x80));

        dirty = false;
    }
}
