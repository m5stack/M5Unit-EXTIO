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
#include <wiring/m5_unit_unified_wiring.hpp>  // Board-aware connection helpers (include last)

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

    // Board-aware connection: NessoN1 -> SoftwareI2C on GROVE, NanoC6/NanoH2 -> M5.Ex_I2C, others -> Wire
    if (!m5::unit::wiring::addI2C(Units, unit) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        m5::unit::wiring::failStop();
    }

    M5_LOGI("M5UnitUnified initialized");
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
