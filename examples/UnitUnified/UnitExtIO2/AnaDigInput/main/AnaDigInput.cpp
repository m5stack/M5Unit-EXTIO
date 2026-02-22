/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Analog/Digital input example using M5UnitUnified for UnitExtIO2

  Operation:
    Click BtnA : Change input mode
*/
#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedEXTIO.h>
#include <M5Utility.h>
#include <M5HAL.hpp>  // For NessoN1

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

// Layout parameters (set in setup based on screen size)
bool can_render{};  // false if no LCD or e-paper
bool small_screen{};
int16_t font_w{8}, font_h{16};
int16_t title_y{}, data_y{}, col_w{}, col_x0{};

void setup_layout()
{
    can_render = (lcd.width() > 0 && lcd.height() > 0 && !lcd.isEPD());
    if (!can_render) {
        return;
    }
    small_screen = (lcd.width() < 200);
    if (small_screen) {
        lcd.setFont(&fonts::Font0);
        font_w = 6;
        font_h = 8;
    } else {
        lcd.setFont(&fonts::AsciiFont8x16);
        font_w = 8;
        font_h = 16;
    }
    title_y = 2;
    data_y  = title_y + font_h + 2;
    col_x0  = 2;
    col_w   = lcd.width() / 2;
}

void change_operation(const Operation op)
{
    latest = 0;
    switch (op) {
        case Operation::Digital:
            unit.writePinBitsMode(target_pins, Mode::DigitalInput);
            break;
        case Operation::Analog8:
        case Operation::Analog12:
            unit.writePinBitsMode(target_pins, Mode::ADCInput);
            break;
        default:
            break;
    }
    if (!can_render) {
        return;
    }
    lcd.fillScreen(TFT_BLACK);
    lcd.setCursor(col_x0, title_y);
    switch (op) {
        case Operation::Digital:
            lcd.printf("%s", small_screen ? "DIG IN" : "DIGITAL INPUT");
            break;
        case Operation::Analog8:
            lcd.printf("%s", small_screen ? "ANA8" : "ANALOG 8 INPUT");
            break;
        case Operation::Analog12:
            lcd.printf("%s", small_screen ? "ANA12" : "ANALOG 12 INPUT");
            break;
        default:
            break;
    }
}

using m5::unit::UnitExtIO2;

void render_status(const uint16_t v[UnitExtIO2::NUMBER_OF_PINS])
{
    if (!can_render) {
        return;
    }
    for (uint8_t pin = 0; pin < UnitExtIO2::NUMBER_OF_PINS; ++pin) {
        if (unit.mode(pin) == Mode::ADCInput) {
            int16_t x = col_x0 + col_w * (pin & 1);
            int16_t y = data_y + font_h * (pin / 2);
            lcd.setCursor(x, y);
            if (small_screen) {
                lcd.printf("%u:%4u", pin, v[pin]);
            } else {
                lcd.printf("Pin%u:%4u", pin, v[pin]);
            }
        }
    }
}

void render_status(const uint8_t high_bits)
{
    if (!can_render) {
        return;
    }
    for (uint8_t pin = 0; pin < UnitExtIO2::NUMBER_OF_PINS; ++pin) {
        if (unit.mode(pin) == Mode::DigitalInput) {
            int16_t x = col_x0 + col_w * (pin & 1);
            int16_t y = data_y + font_h * (pin / 2);
            bool h    = (1U << pin) & high_bits;
            lcd.setCursor(x, y);
            if (small_screen) {
                lcd.printf("%u:", pin);
                lcd.setTextColor(h ? TFT_BLUE : TFT_YELLOW, TFT_BLACK);
                lcd.printf("%s", h ? "H" : "L");
            } else {
                lcd.printf("Pin%u:", pin);
                lcd.setCursor(x + font_w * 5, y);
                lcd.setTextColor(h ? TFT_BLUE : TFT_YELLOW, TFT_BLACK);
                lcd.printf("%s", h ? "HIGH" : "LOW ");
            }
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
        lcd.startWrite();
        render_status(v);
        lcd.endWrite();
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
        lcd.startWrite();
        render_status(v);
        lcd.endWrite();
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
        lcd.startWrite();
        render_status(high_bits);
        lcd.endWrite();
        M5.Log.printf("DIG:%X\n", high_bits);
    }
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

    setup_layout();

    change_operation(operation);
}

void loop()
{
    M5.update();

    // Change ANA8 -> ANA12 -> DIG -> ANA8 ...
    if (M5.BtnA.wasClicked()) {
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
