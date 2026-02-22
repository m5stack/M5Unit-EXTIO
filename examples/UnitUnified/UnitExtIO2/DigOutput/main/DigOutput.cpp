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
#include <M5HAL.hpp>  // For NessoN1

using namespace m5::unit::extio2;
using m5::unit::UnitExtIO2;

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;
m5::unit::UnitExtIO2 unit;

uint8_t state{};
uint8_t current{};

void render()
{
    lcd.drawString("DIGITAL OUTPUT", 8, 8);

    lcd.setCursor(16, 32);
    lcd.printf("Pin0:%u%u%u%u%u%u%u%u:Pin7", (bool)(state & 0x01), (bool)(state & 0x02), (bool)(state & 0x04),
               (bool)(state & 0x08), (bool)(state & 0x10), (bool)(state & 0x20), (bool)(state & 0x40),
               (bool)(state & 0x80));
    lcd.setCursor(16, 32 + 16);
    lcd.printf("Current Pin:%u", current);
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

    auto board       = M5.getBoard();
    auto pin_num_sda = M5.getPin(m5::pin_name_t::port_a_sda);
    auto pin_num_scl = M5.getPin(m5::pin_name_t::port_a_scl);

    // For NessoN1 GROVE
    if (board == m5::board_t::board_ArduinoNessoN1) {
        pin_num_sda = M5.getPin(m5::pin_name_t::port_b_out);
        pin_num_scl = M5.getPin(m5::pin_name_t::port_b_in);
        M5_LOGI("getPin(NessoN1): SDA:%u SCL:%u", pin_num_sda, pin_num_scl);
        m5::hal::bus::I2CBusConfig i2c_cfg;
        i2c_cfg.pin_sda = m5::hal::gpio::getPin(pin_num_sda);
        i2c_cfg.pin_scl = m5::hal::gpio::getPin(pin_num_scl);
        auto i2c_bus    = m5::hal::bus::i2c::getBus(i2c_cfg);
        if (!Units.add(unit, i2c_bus ? i2c_bus.value() : nullptr) || !Units.begin()) {
            M5_LOGE("Failed to begin");
            lcd.fillScreen(TFT_RED);
            while (true) {
                m5::utility::delay(10000);
            }
        }
    } else {
        M5_LOGI("getPin: SDA:%u SCL:%u", pin_num_sda, pin_num_scl);
        Wire.end();
        Wire.begin(pin_num_sda, pin_num_scl, 100 * 1000U);
        if (!Units.add(unit, Wire) || !Units.begin()) {
            M5_LOGE("Failed to begin");
            lcd.fillScreen(TFT_RED);
            while (true) {
                m5::utility::delay(10000);
            }
        }
    }

    M5_LOGI("M5UnitUnified has been begun");
    M5_LOGI("%s", Units.debugInfo().c_str());

    lcd.setFont(&fonts::AsciiFont8x16);
    lcd.startWrite();
    lcd.fillScreen(TFT_BLACK);

    unit.writeAllMode(Mode::DigitalOutput);
    unit.writeAllDigitalOutput(false);  // All pins to LOW
    lcd.endWrite();
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
        lcd.startWrite();
        render();
        lcd.endWrite();
        M5.Log.printf("Output Pin0:%u%u%u%u%u%u%u%u:Pin7\n", (bool)(state & 0x01), (bool)(state & 0x02),
                      (bool)(state & 0x04), (bool)(state & 0x08), (bool)(state & 0x10), (bool)(state & 0x20),
                      (bool)(state & 0x40), (bool)(state & 0x80));

        dirty = false;
    }
}
