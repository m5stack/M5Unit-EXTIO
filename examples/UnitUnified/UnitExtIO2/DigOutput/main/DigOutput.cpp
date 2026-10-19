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

// Bits as "01010101" in the order Pin0 ... Pin7
void bits_to_string(char out[UnitExtIO2::NUMBER_OF_PINS + 1], const uint8_t bits)
{
    for (uint8_t pin = 0; pin < UnitExtIO2::NUMBER_OF_PINS; ++pin) {
        out[pin] = ((bits >> pin) & 1) ? '1' : '0';
    }
    out[UnitExtIO2::NUMBER_OF_PINS] = '\0';
}

void render()
{
    if (!can_render) {
        return;
    }
    lcd.drawString(small_screen ? "DIG OUT" : "DIGITAL OUTPUT", 2, 2);

    int16_t row1 = 2 + font_h + 2;
    int16_t row2 = row1 + font_h + 2;
    char bits[UnitExtIO2::NUMBER_OF_PINS + 1]{};
    bits_to_string(bits, state);
    lcd.setCursor(2, row1);
    lcd.printf(small_screen ? "%s" : "Pin0:%s:Pin7", bits);
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

    if (!unit.writeAllMode(Mode::DigitalOutput)) {
        M5_LOGE("Failed to set mode");
        m5::unit::wiring::failStop();
    }
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
        char bits[UnitExtIO2::NUMBER_OF_PINS + 1]{};
        bits_to_string(bits, state);
        M5.Log.printf("Output Pin0:%s:Pin7\n", bits);

        dirty = false;
    }
}

#if !defined(ARDUINO)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_timer.h>

#if CONFIG_FREERTOS_UNICORE
// Single-core SoCs: run loop() back-to-back, but every ~2 s yield a 5 ms slice so the IDLE task
// runs and feeds the task watchdog (default 5 s).
static inline void feedIdleTaskPeriodically(void)
{
    constexpr uint32_t FEED_INTERVAL_MS{2000};
    constexpr TickType_t FEED_SLEEP_TICKS{pdMS_TO_TICKS(5)};
    static uint32_t s_last_feed_ms{};
    const uint32_t now_ms{static_cast<uint32_t>(esp_timer_get_time() / 1000)};
    if (now_ms - s_last_feed_ms >= FEED_INTERVAL_MS) {
        s_last_feed_ms = now_ms;
        vTaskDelay(FEED_SLEEP_TICKS);
    }
}
#endif

extern "C" void app_main(void)
{
    setup();
    for (;;) {
#if CONFIG_FREERTOS_UNICORE
        feedIdleTaskPeriodically();
#endif
        loop();
    }
}
#endif
