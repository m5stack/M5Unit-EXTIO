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

    unit.writePinBitsMode(target_pins, Mode::ServoControl);

    lcd.drawString("SERVO CONTROL", 8, 8);
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

    for (uint16_t pulse = 500; pulse <= 2500; pulse += 100) {
        if (!unit.writePinBitsServoPulse(target_pins, pulse)) {
            M5_LOGE("Failed to write");
        }
        lcd.setCursor(16, 48);
        lcd.printf("Pulse:%4u        ", pulse);
        m5::utility::delay(100);
    }
    m5::utility::delay(1000);

    for (uint8_t deg = 0; deg <= 180; deg += 45) {
        if (!unit.writePinBitsServoAngle(target_pins, deg)) {
            M5_LOGE("Failed to write");
        }
        lcd.setCursor(16, 48);
        lcd.printf("Angle:%3u(deg)", deg);
        m5::utility::delay(500);
    }
    m5::utility::delay(1000);
}
