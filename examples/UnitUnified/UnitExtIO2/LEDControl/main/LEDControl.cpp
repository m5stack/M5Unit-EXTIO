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

    unit.writePinBitsMode(target_pins, Mode::LEDControl);

    lcd.drawString("LED CONTROL", 8, 8);
    std::string s = "TargetPins:";
    for (uint8_t pin = 0; pin < UnitExtIO2::NUMBER_OF_PINS; ++pin) {
        if ((1U << pin) & target_pins) {
            s += m5::utility::formatString("%u ", pin);
        }
    }
    lcd.drawString(s.c_str(), 16, 32);
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

    lcd.setCursor(16, 32 + 16);
    lcd.printf("RGB(%3u,%3u,%3u)", r, g, b);
    M5.Log.printf("RGB(%3u,%3u,%3u)\n", r, g, b);
    lcd.fillRect(16, 32 * 2, lcd.width(), 16, (uint16_t)lgfx::rgb565_t(r, g, b));

    ++counter;
}
