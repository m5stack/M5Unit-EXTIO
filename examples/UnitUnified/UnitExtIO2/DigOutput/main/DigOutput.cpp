/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Digital output example using M5UnitUnified for UnitExtIO2

  Operation:
    Click BtnA or screen (if the touch screen is enabled) : Toggle HIGH/LOW on current pin
    Hold  BtnA or screen (if the touch screen is enabled) : Current pin to next
*/
#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedEXTIO.h>
#include <M5Utility.h>

using namespace m5::unit::extio2;
using m5::unit::UnitExtIO2;

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;
m5::unit::UnitExtIO2 unit;

uint8_t state{};
uint32_t current{};

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
    // The screen shall be in landscape mode
    if (lcd.height() > lcd.width()) {
        lcd.setRotation(1);
    }

    auto pin_num_sda = M5.getPin(m5::pin_name_t::port_a_sda);
    auto pin_num_scl = M5.getPin(m5::pin_name_t::port_a_scl);
    M5_LOGI("getPin: SDA:%u SCL:%u", pin_num_sda, pin_num_scl);
    Wire.end();
    Wire.begin(pin_num_sda, pin_num_scl, 100 * 1000U);

    if (!Units.add(unit, Wire) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        lcd.clear(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }
    M5_LOGI("M5UnitUnified has been begun");
    M5_LOGI("%s", Units.debugInfo().c_str());

    lcd.setFont(&fonts::AsciiFont8x16);
    lcd.startWrite();
    lcd.clear();

    unit.writeAllMode(Mode::DigitalOutput);
    unit.writeAllDigitalOutput(false);  // All pins to LOW
    M5.Log.printf("Current Pin:%u\n", current);
}

void loop()
{
    static bool dirty{true};

    M5.update();
    auto touch = M5.Touch.getDetail();

    if (M5.BtnA.wasClicked() || touch.wasClicked()) {
        // Toggle HIGH/LOW
        state ^= (1U << current);
        M5.Log.printf("Change Pin:%u to %s\n", current, (state & (1U << current)) ? "HIGH" : "LOW");
        dirty = true;
    } else if (M5.BtnA.wasHold() || touch.wasHold()) {
        // Change current to next
        current = (current + 1) % UnitExtIO2::NUMBER_OF_PINS;
        M5.Log.printf("Current Pin:%u\n", current);
        dirty = true;
    }

    if (dirty) {
        if (!unit.writePinBitsDigitalOutput(0xFF /* All pins */, state)) {
            M5_LOGE("Failed to output");
        }
        render();
        M5.Log.printf("Output Pin0:%u%u%u%u%u%u%u%u:Pin7\n", (bool)(state & 0x01), (bool)(state & 0x02),
                      (bool)(state & 0x04), (bool)(state & 0x08), (bool)(state & 0x10), (bool)(state & 0x20),
                      (bool)(state & 0x40), (bool)(state & 0x80));

        dirty = false;
    }
}
