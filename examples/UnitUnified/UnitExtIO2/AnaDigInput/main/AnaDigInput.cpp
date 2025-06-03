/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Analog/Digital input example using M5UnitUnified for UnitExtIO2

  Operation:
    Click BtnA or screen (if the touch screen is enabled) : Change input mode
*/
#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedEXTIO.h>
#include <M5Utility.h>

using namespace m5::unit::extio2;

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;
m5::unit::UnitExtIO2 unit;

enum class Operation : uint8_t { Analog8, Analog12, Digital };
Operation& operator++(Operation& op)
{
    op = static_cast<Operation>((m5::stl::to_underlying(op) + 1) % 3);
    return op;
}
Operation operation{Operation::Analog8};
unsigned long latest{};

constexpr uint32_t interval{20};  // Interval for reading values (ms)

// Bits of the target pin (LSB:pin0 ... pin7:MSB)
// e.g.
//   0X01 Pin 0
//   0X9C Pin 2,3,4,7
//   0XFF All pins
constexpr uint8_t target_pins{0xFF};
static_assert(target_pins != 0, "Specify at least one");

void change_operation(const Operation op)
{
    latest = 0;
    lcd.clear(TFT_BLACK);
    lcd.setCursor(8, 8);
    switch (op) {
        case Operation::Digital:
            unit.writePinBitsMode(target_pins, Mode::DigitalInput);
            lcd.printf("%s", "DIGITAL INPUT");
            break;
        case Operation::Analog8:
            unit.writePinBitsMode(target_pins, Mode::ADCInput);
            lcd.printf("%s", "ANALOG 8 INPUT");
            break;
        case Operation::Analog12:
            unit.writePinBitsMode(target_pins, Mode::ADCInput);
            lcd.printf("%s", "ANALOG 12 INPUT");
            break;
        default:
            break;
    }
}

using m5::unit::UnitExtIO2;

void render_status(const uint16_t v[UnitExtIO2::NUMBER_OF_PINS])
{
    for (uint8_t pin = 0; pin < UnitExtIO2::NUMBER_OF_PINS; ++pin) {
        if (unit.mode(pin) == Mode::ADCInput) {
            lcd.setCursor(16 + 120 * (pin & 1), 24 + 16 * (pin / 2));
            lcd.printf("Pin%u:%4u", pin, v[pin]);
        }
    }
}

void render_status(const uint8_t high_bits)
{
    for (uint8_t pin = 0; pin < UnitExtIO2::NUMBER_OF_PINS; ++pin) {
        if (unit.mode(pin) == Mode::DigitalInput) {
            int32_t x = 16 + 80 * (pin & 1);
            int32_t y = 24 + 16 * (pin / 2);
            bool h    = (1U << pin) & high_bits;
            lcd.setCursor(x, y);
            lcd.printf("Pin%u:", pin);
            lcd.setCursor(x + 8 * 5, y);
            lcd.setTextColor(h ? TFT_BLUE : TFT_YELLOW, TFT_BLACK);
            lcd.printf("%s", h ? "HIGH" : "LOW ");
            lcd.setTextColor(TFT_WHITE, TFT_BLACK);
        }
    }
}

void analog8_function()
{
    uint16_t v[UnitExtIO2::NUMBER_OF_PINS]{};
    auto at = m5::utility::millis();
    if (!latest || at >= latest + interval) {
        if (!unit.readPinBitsAnalogInput8(v, target_pins)) {
            M5_LOGE("Failed to read");
            return;
        }
        latest = at;
        render_status(v);
        M5.Log.printf("ANA8:%3u,%3u,%3u,%3u,%3u,%3u,%3u,%3u\n", v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7]);
    }
}

void analog12_function()
{
    uint16_t v[UnitExtIO2::NUMBER_OF_PINS]{};

    auto at = m5::utility::millis();
    if (!latest || at >= latest + interval) {
        if (!unit.readPinBitsAnalogInput12(v, target_pins)) {
            M5_LOGE("Failed to read");
            return;
        }
        latest = at;
        render_status(v);
        M5.Log.printf("ANA12:%4u,%4u,%4u,%4u,%4u,%4u,%4u,%4u\n", v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7]);
    }
}

void digital_function()
{
    uint8_t high_bits{};

    auto at = m5::utility::millis();
    if (!latest || at >= latest + interval) {
        if (!unit.readPinBitsDigitalInput(high_bits, target_pins)) {
            M5_LOGE("Failed to read");
            return;
        }
        latest = at;
        render_status(high_bits);
        M5.Log.printf("DIG:%X\n", high_bits);
    }
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

    change_operation(operation);
}

void loop()
{
    M5.update();
    auto touch = M5.Touch.getDetail();

    // Change DIG -> ANA8 -> ANA12 -> DIG...
    if (M5.BtnA.wasClicked() || touch.wasClicked()) {
        change_operation(++operation);
        return;
    }

    switch (operation) {
        case Operation::Digital:
            digital_function();
            break;
        case Operation::Analog8:
            analog8_function();
            break;
        case Operation::Analog12:
            analog12_function();
            break;
        default:
            break;
    }
}
