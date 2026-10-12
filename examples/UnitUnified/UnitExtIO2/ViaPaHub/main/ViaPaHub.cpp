/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example of using UnitExtIO2 via UnitPaHub

  Board ---> PaHub ---> ch:0 UnitExtIO2
  All pins are set to ADC input on begin.
  Prints the 12-bit ADC values of all pins to the serial output (Serial Plotter format),
  and draws them when the board has a screen.
*/
#include <algorithm>
#include <cstdio>

#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedEXTIO.h>
#include <M5Utility.h>
#include <M5UnitUnifiedHUB.h>                 // UnitPaHub
#include <wiring/m5_unit_unified_wiring.hpp>  // Board-aware connection helpers (include last)

using namespace m5::unit::extio2;

namespace {
auto& lcd = M5.Display;

m5::unit::UnitUnified Units;
m5::unit::UnitPaHub hub;
m5::unit::UnitExtIO2 unit;

constexpr uint8_t UNIT_CHANNEL{0};    // PaHub channel the unit is connected to
constexpr uint32_t INTERVAL_MS{100};  // Interval for reading values (ms)
constexpr uint32_t ROWS{4};           // Two pins per row
m5::utility::elapsed_time_t latest_ms{};

//! True when the board has a real screen (Atom / NanoC6 / NanoH2 / NessoN1 have none)
bool has_lcd{};

//! The values of all pins, drawn off-screen and pushed at once to avoid flicker
M5Canvas canvas(&lcd);

void setup_canvas()
{
    lcd.fillScreen(TFT_DARKGREEN);
    // 6x8 font scaled so that 14 characters ("P0:4095 P1:4095") fit the width
    const int32_t text_size = std::max<int32_t>(1, lcd.width() / (15 * 6));
    canvas.setPsram(false);
    canvas.setColorDepth(1);  // Two colors: background and text
    canvas.createSprite(lcd.width(), 8 * text_size * ROWS);
    canvas.setPaletteColor(0, TFT_DARKGREEN);
    canvas.setPaletteColor(1, TFT_YELLOW);
    canvas.setTextSize(text_size);
    canvas.setTextDatum(top_center);
    canvas.setTextColor(1);
}

void draw_values(const uint16_t values[m5::unit::UnitExtIO2::NUMBER_OF_PINS])
{
    const int32_t line_h = canvas.height() / ROWS;
    canvas.fillSprite(0);
    for (uint32_t row = 0; row < ROWS; ++row) {
        char buf[20]{};
        const uint32_t pin = row * 2;
        snprintf(buf, sizeof(buf), "P%u:%4u P%u:%4u", (unsigned)pin, values[pin], (unsigned)(pin + 1), values[pin + 1]);
        canvas.drawString(buf, canvas.width() / 2, line_h * row);
    }
    canvas.pushSprite(0, (lcd.height() - canvas.height()) / 2);
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

    // All pins are set to ADC input on begin
    auto cfg = unit.config();
    std::fill(std::begin(cfg.mode), std::end(cfg.mode), Mode::ADCInput);
    unit.config(cfg);

    if (!hub.add(unit, UNIT_CHANNEL)) {  // PaHub ch:0 -> UnitExtIO2
        M5_LOGE("Failed to add children");
        m5::unit::wiring::failStop();
    }
    // Board-aware I2C for the PaHub: NessoN1 -> SoftwareI2C on GROVE, NanoC6/NanoH2 -> M5.Ex_I2C, others -> Wire.
    // The unit is reached through the hub (added above), so only the hub is added here.
    if (!m5::unit::wiring::addI2C(Units, hub) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        m5::unit::wiring::failStop();
    }

    M5_LOGI("M5UnitUnified initialized");
    M5_LOGI("%s", Units.debugInfo().c_str());

    // A board without a screen reports a 1x1 dummy display
    has_lcd = (lcd.width() > 8 && lcd.height() > 8);
    if (has_lcd) {
        setup_canvas();
    }
}

void loop()
{
    M5.update();
    Units.update();

    if (latest_ms && !m5::utility::hasElapsed(latest_ms, INTERVAL_MS)) {
        return;
    }
    latest_ms = m5::utility::millis();

    uint16_t values[m5::unit::UnitExtIO2::NUMBER_OF_PINS]{};
    if (!unit.readAllAnalogInput12(values)) {
        M5_LOGE("Failed to read");
        return;
    }
    // Serial Plotter format
    M5.Log.printf(">P0:%u\n>P1:%u\n>P2:%u\n>P3:%u\n>P4:%u\n>P5:%u\n>P6:%u\n>P7:%u\n", values[0], values[1], values[2],
                  values[3], values[4], values[5], values[6], values[7]);
    if (has_lcd) {
        draw_values(values);
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
