/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Servo control example using M5UnitUnified for UnitExtIO2
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

// Bits of the target pin (LSB:pin0 ... pin7:MSB)
// e.g.
//   0X01 Pin 0
//   0X9C Pin 2,3,4,7
//   0XFF All pins
constexpr uint8_t target_pins{0xFF};
static_assert(target_pins != 0, "Specify at least one");

bool can_render{};
bool small_screen{};
int16_t font_h{16};
int16_t data_y{};

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

    unit.writePinBitsMode(target_pins, Mode::ServoControl);

    if (can_render) {
        if (small_screen) {
            lcd.setFont(&fonts::Font0);
            font_h = 8;
        } else {
            lcd.setFont(&fonts::AsciiFont8x16);
            font_h = 16;
        }
        data_y = 2 + (font_h + 2) * 2;

        lcd.startWrite();
        lcd.fillScreen(TFT_BLACK);

        lcd.drawString(small_screen ? "SERVO" : "SERVO CONTROL", 2, 2);
        std::string s = small_screen ? "P:" : "TargetPins:";
        for (uint8_t pin = 0; pin < UnitExtIO2::NUMBER_OF_PINS; ++pin) {
            if ((1U << pin) & target_pins) {
                s += m5::utility::formatString("%u", pin);
                if (!small_screen) {
                    s += ' ';
                }
            }
        }
        lcd.drawString(s.c_str(), 2, 2 + font_h + 2);
        lcd.endWrite();
    }
}

void loop()
{
    M5.update();

    for (uint16_t pulse = 500; pulse <= 2500; pulse += 100) {
        if (!unit.writePinBitsServoPulse(target_pins, pulse)) {
            M5_LOGE("Failed to write");
        }
        if (can_render) {
            lcd.startWrite();
            lcd.setCursor(2, data_y);
            lcd.printf(small_screen ? "P:%4u " : "Pulse:%4u        ", pulse);
            lcd.endWrite();
        }
        m5::utility::delay(100);
    }
    m5::utility::delay(1000);

    for (uint8_t deg = 0; deg <= 180; deg += 45) {
        if (!unit.writePinBitsServoAngle(target_pins, deg)) {
            M5_LOGE("Failed to write");
        }
        if (can_render) {
            lcd.startWrite();
            lcd.setCursor(2, data_y);
            lcd.printf(small_screen ? "A:%3ud " : "Angle:%3u(deg)", deg);
            lcd.endWrite();
        }
        M5.Log.printf("Angle:%3u(deg)\n", deg);
        m5::utility::delay(500);
    }
    m5::utility::delay(1000);
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
