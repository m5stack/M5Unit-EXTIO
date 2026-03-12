/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  UnitTest for UnitExtIO2
*/
#include <array>
#include <ostream>
template <typename T, size_t N>
std::ostream& operator<<(std::ostream& os, const std::array<T, N>& arr)
{
    os << "[";
    for (size_t i = 0; i < N; ++i) {
        os << arr[i];
        if (i < N - 1) {
            os << ", ";
        }
    }
    os << "]";
    return os;
}
#include <gtest/gtest.h>
#include <Wire.h>
#include <M5Unified.h>
#include <M5UnitUnified.hpp>
#include <googletest/test_template.hpp>
#include <googletest/test_helper.hpp>
#include <unit/unit_ExtIO2.hpp>
#include <esp_random.h>

using namespace m5::unit::googletest;
using namespace m5::unit;
using namespace m5::unit::extio2;
using m5::unit::types::elapsed_time_t;

class TestExtIO2 : public I2CComponentTestBase<UnitExtIO2> {
protected:
    virtual UnitExtIO2* get_instance() override
    {
        auto ptr = new m5::unit::UnitExtIO2();
        return ptr;
    }
};

namespace {

uint8_t random_servo_angle()
{
    return esp_random() % (UnitExtIO2::MAX_SERVO_ANGLE - UnitExtIO2::MIN_SERVO_ANGLE + 1) + UnitExtIO2::MIN_SERVO_ANGLE;
}
uint16_t random_servo_pulse()
{
    return esp_random() % (UnitExtIO2::MAX_SERVO_PULSE - UnitExtIO2::MIN_SERVO_PULSE + 1) + UnitExtIO2::MIN_SERVO_PULSE;
}

constexpr Mode mode_table[] = {
    Mode::DigitalInput, Mode::DigitalOutput, Mode::ADCInput, Mode::ServoControl, Mode::LEDControl,
};

constexpr uint8_t pin_bits_table[] = {
    0x00,  // All off
    0x01,  // Pin 0
    0x02,  // Pin 1
    0x04,  // Pin 2
    0x08,  // Pin 3
    0x10,  // Pin 4
    0x20,  // Pin 5
    0x40,  // Pin 6
    0x80,  // Pin 7
    0x03,  // Adjacent bits (0,1)
    0x0C,  // Adjacent bits (2,3)
    0xC0,  // Adjacent bits (6,7)
    0x24,  // Bits that are far apart (2,5)
    0x81,  // Bits that are far apart (0,7)
    0x55,  // Odd bits
    0xAA,  // Even bits
    0xFF,  // All on
};

bool check_mode(const Mode mode[UnitExtIO2::NUMBER_OF_PINS], const uint8_t pin_bits, const Mode m)
{
    for (uint8_t pin = 0; pin < UnitExtIO2::NUMBER_OF_PINS; ++pin) {
        if ((1U << pin) & pin_bits) {
            if (mode[pin] != m) {
                return false;
            }
        }
    }
    return true;
}

template <typename T, size_t N>
bool check_values(const std::array<T, N>& arr, const uint8_t pin_bits)
{
    for (uint8_t pin = 0; pin < UnitExtIO2::NUMBER_OF_PINS; ++pin) {
        if ((1U << pin) & pin_bits) {
            if (!arr[pin]) {
                return false;
            }
        }
    }
    return true;
}

template <typename T, size_t N>
bool check_values_eq(const std::array<T, N>& arr, const uint8_t pin_bits, const T v)
{
    for (uint8_t pin = 0; pin < UnitExtIO2::NUMBER_OF_PINS; ++pin) {
        if ((1U << pin) & pin_bits) {
            if (arr[pin] != v) {
                return false;
            }
        }
    }
    return true;
}

}  // namespace

TEST_F(TestExtIO2, FirmwareVersion)
{
    SCOPED_TRACE(ustr);

    uint8_t ver{};
    EXPECT_TRUE(unit->readFirmwareVersion(ver));
    EXPECT_EQ(ver, unit->firmwareVersion());
    EXPECT_NE(ver, 0);
}

TEST_F(TestExtIO2, Mode)
{
    SCOPED_TRACE(ustr);

    // Check initial state
    {
        std::array<Mode, UnitExtIO2::NUMBER_OF_PINS> mm{};
        EXPECT_TRUE(unit->readAllMode(mm.data()));
        EXPECT_TRUE(std::all_of(mm.begin(), mm.end(), [](const Mode m) { return m == Mode::DigitalInput; }));
    }

    Mode m{};
    constexpr uint8_t invalid_pin{UnitExtIO2::NUMBER_OF_PINS};
    EXPECT_FALSE(unit->readMode(m, invalid_pin));
    EXPECT_FALSE(unit->writeMode(invalid_pin, m));

    EXPECT_FALSE(unit->writePinBitsMode(0x01, Mode::Invalid));
    EXPECT_FALSE(unit->writeAllMode(Mode::Invalid));

    // Each
    for (uint8_t pin = 0; pin < UnitExtIO2::NUMBER_OF_PINS; ++pin) {
        for (auto&& m : mode_table) {
            auto s = m5::utility::formatString("PIN:%u Mode:%u", pin, m);
            SCOPED_TRACE(s);

            EXPECT_TRUE(unit->writeMode(pin, m));
            Mode mm{};
            EXPECT_TRUE(unit->readMode(mm, pin));
            EXPECT_EQ(mm, m);
            EXPECT_EQ(unit->mode(pin), m);
        }
    }

    // Pin bits
    for (auto&& pin_bits : pin_bits_table) {
        for (auto&& m : mode_table) {
            auto s = m5::utility::formatString("PIN:%X Mode:%u", pin_bits, m);
            SCOPED_TRACE(s);
            std::array<Mode, UnitExtIO2::NUMBER_OF_PINS> ma{};

            if (pin_bits) {
                EXPECT_TRUE(unit->writePinBitsMode(pin_bits, m));
                EXPECT_TRUE(unit->readAllMode(ma.data()));
                EXPECT_TRUE(check_mode(ma.data(), pin_bits, m));

            } else {
                EXPECT_TRUE(unit->readAllMode(ma.data()));
                EXPECT_FALSE(unit->writePinBitsMode(pin_bits, m));
                std::array<Mode, UnitExtIO2::NUMBER_OF_PINS> mb{};
                EXPECT_TRUE(unit->readAllMode(mb.data()));
                EXPECT_EQ(mb, ma);
            }
        }
    }

    // All (single Mode)
    for (auto&& m : mode_table) {
        EXPECT_TRUE(unit->writeAllMode(m));

        Mode ma[UnitExtIO2::NUMBER_OF_PINS]{};
        EXPECT_TRUE(unit->readAllMode(ma));
        EXPECT_TRUE(std::all_of(std::begin(ma), std::end(ma), [&m](const Mode md) { return md == m; }));

        for (uint8_t pin = 0; pin < UnitExtIO2::NUMBER_OF_PINS; ++pin) {
            EXPECT_EQ(unit->mode(pin), m);
        }
    }

    // All (Mode array)
    {
        Mode arr[UnitExtIO2::NUMBER_OF_PINS] = {
            Mode::DigitalInput, Mode::DigitalOutput, Mode::ADCInput,      Mode::ServoControl,
            Mode::LEDControl,   Mode::DigitalInput,  Mode::DigitalOutput, Mode::ADCInput,
        };
        EXPECT_TRUE(unit->writeAllMode(arr));
        Mode ra[UnitExtIO2::NUMBER_OF_PINS]{};
        EXPECT_TRUE(unit->readAllMode(ra));
        EXPECT_TRUE(std::equal(std::begin(arr), std::end(arr), std::begin(ra)));
    }
}

TEST_F(TestExtIO2, DigitalInput)
{
    SCOPED_TRACE(ustr);

    bool high{};
    uint8_t high_bits{};

    EXPECT_TRUE(unit->writeMode(0, Mode::DigitalOutput));
    EXPECT_FALSE(unit->readDigitalInput(high, 0));
    EXPECT_FALSE(unit->readAllDigitalInput(high_bits));

    for (auto&& pin_bits : pin_bits_table) {
        auto s = m5::utility::formatString("PIN:%X", pin_bits);
        SCOPED_TRACE(s);
        if (!pin_bits) {
            continue;
        }
        EXPECT_TRUE(unit->writePinBitsMode(pin_bits, Mode::DigitalInput));
        EXPECT_TRUE(unit->readPinBitsDigitalInput(high_bits, pin_bits));
    }
}

TEST_F(TestExtIO2, DigitalOutput)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->writeMode(0, Mode::DigitalInput));
    EXPECT_FALSE(unit->writeDigitalOutput(0, true));
    EXPECT_FALSE(unit->writeDigitalOutput(0, false));
    EXPECT_FALSE(unit->writeAllDigitalOutput(true));
    EXPECT_FALSE(unit->writeAllDigitalOutput(false));

    for (auto&& pin_bits : pin_bits_table) {
        auto s = m5::utility::formatString("PIN:%X", pin_bits);
        SCOPED_TRACE(s);
        if (!pin_bits) {
            continue;
        }
        EXPECT_TRUE(unit->writePinBitsMode(pin_bits, Mode::DigitalOutput));
        EXPECT_TRUE(unit->writePinBitsDigitalOutput(pin_bits, true));
        EXPECT_TRUE(unit->writePinBitsDigitalOutput(pin_bits, false));
        EXPECT_TRUE(unit->writePinBitsDigitalOutput(pin_bits, 0xFF));
        EXPECT_TRUE(unit->writePinBitsDigitalOutput(pin_bits, 0x00));
    }
}

TEST_F(TestExtIO2, ADCInput)
{
    SCOPED_TRACE(ustr);

    std::array<uint16_t, UnitExtIO2::NUMBER_OF_PINS> values{};

    EXPECT_TRUE(unit->writeMode(0, Mode::DigitalInput));
    EXPECT_FALSE(unit->readAllAnalogInput8(values.data()));
    EXPECT_FALSE(unit->readAllAnalogInput12(values.data()));

    for (auto&& pin_bits : pin_bits_table) {
        auto s = m5::utility::formatString("PIN:%X", pin_bits);
        SCOPED_TRACE(s);
        if (!pin_bits) {
            continue;
        }
        EXPECT_TRUE(unit->writePinBitsMode(pin_bits, Mode::ADCInput));

        EXPECT_TRUE(unit->readPinBitsAnalogInput8(values.data(), pin_bits));
        EXPECT_TRUE(check_values(values, pin_bits)) << values;

        EXPECT_TRUE(unit->readPinBitsAnalogInput12(values.data(), pin_bits));
        EXPECT_TRUE(check_values(values, pin_bits)) << values;
    }
}

TEST_F(TestExtIO2, ServoControl)
{
    SCOPED_TRACE(ustr);

    std::array<uint8_t, UnitExtIO2::NUMBER_OF_PINS> angles{};
    std::array<uint16_t, UnitExtIO2::NUMBER_OF_PINS> pulses{};

    EXPECT_TRUE(unit->writeMode(0, Mode::DigitalInput));
    EXPECT_FALSE(unit->readAllServoAngle(angles.data()));
    EXPECT_FALSE(unit->readAllServoPulse(pulses.data()));
    EXPECT_FALSE(unit->writeAllServoAngle(0));
    EXPECT_FALSE(unit->writeAllServoPulse(0));

    // Boundary values (once with all pins)
    EXPECT_TRUE(unit->writeAllMode(Mode::ServoControl));
    EXPECT_TRUE(unit->writePinBitsServoAngle(0xFF, 0));
    EXPECT_TRUE(unit->writePinBitsServoAngle(0xFF, 90));
    EXPECT_TRUE(unit->writePinBitsServoAngle(0xFF, 180));
    EXPECT_FALSE(unit->writePinBitsServoAngle(0xFF, 181));
    EXPECT_FALSE(unit->writePinBitsServoAngle(0xFF, 255));

    EXPECT_FALSE(unit->writePinBitsServoPulse(0xFF, 0));
    EXPECT_FALSE(unit->writePinBitsServoPulse(0xFF, 499));
    EXPECT_TRUE(unit->writePinBitsServoPulse(0xFF, 500));
    EXPECT_TRUE(unit->writePinBitsServoPulse(0xFF, 1500));
    EXPECT_TRUE(unit->writePinBitsServoPulse(0xFF, 2500));
    EXPECT_FALSE(unit->writePinBitsServoPulse(0xFF, 2501));
    EXPECT_FALSE(unit->writePinBitsServoPulse(0xFF, 65535));

    // Write/read with various pin patterns
    for (auto&& pin_bits : pin_bits_table) {
        auto s = m5::utility::formatString("PIN:%X", pin_bits);
        SCOPED_TRACE(s);
        if (!pin_bits) {
            continue;
        }
        EXPECT_TRUE(unit->writePinBitsMode(pin_bits, Mode::ServoControl));

        uint8_t deg = random_servo_angle();
        auto ds     = m5::utility::formatString("Angle:%u", deg);
        SCOPED_TRACE(ds);
        EXPECT_TRUE(unit->writePinBitsServoAngle(pin_bits, deg));
        std::array<uint8_t, UnitExtIO2::NUMBER_OF_PINS> ra{};
        EXPECT_TRUE(unit->readPinBitsServoAngle(ra.data(), pin_bits));
        EXPECT_TRUE(check_values_eq(ra, pin_bits, deg)) << ra;

        uint16_t pls = random_servo_pulse();
        auto ps      = m5::utility::formatString("Pulse:%u", pls);
        SCOPED_TRACE(ps);
        EXPECT_TRUE(unit->writePinBitsServoPulse(pin_bits, pls));
        std::array<uint16_t, UnitExtIO2::NUMBER_OF_PINS> rp{};
        EXPECT_TRUE(unit->readPinBitsServoPulse(rp.data(), pin_bits));
        // pulse internally stores the value divided by 10
        EXPECT_TRUE(check_values_eq(rp, pin_bits, (uint16_t)(pls / 10 * 10))) << rp;
    }
}

TEST_F(TestExtIO2, LEDControl)
{
    SCOPED_TRACE(ustr);

    std::array<uint32_t, UnitExtIO2::NUMBER_OF_PINS> colors{};
    EXPECT_TRUE(unit->writeMode(0, Mode::DigitalInput));
    EXPECT_FALSE(unit->readAllLEDColor(colors.data()));
    EXPECT_FALSE(unit->writeAllLEDColor(0xFF00FF));

    for (auto&& pin_bits : pin_bits_table) {
        uint32_t color = esp_random() & 0xFFFFFF;
        auto s         = m5::utility::formatString("PIN:%X Clr:%X", pin_bits, color);
        SCOPED_TRACE(s);
        if (!pin_bits) {
            continue;
        }
        EXPECT_TRUE(unit->writePinBitsMode(pin_bits, Mode::LEDControl));

        EXPECT_TRUE(unit->writePinBitsLEDColor(pin_bits, color));

        std::array<uint32_t, UnitExtIO2::NUMBER_OF_PINS> colors{};
        EXPECT_TRUE(unit->readPinBitsLEDColor(colors.data(), pin_bits));
        EXPECT_TRUE(check_values_eq(colors, pin_bits, color)) << colors;
    }
}

TEST_F(TestExtIO2, SinglePinADC)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->writeAllMode(Mode::ADCInput));

    for (uint8_t pin = 0; pin < UnitExtIO2::NUMBER_OF_PINS; ++pin) {
        auto s = m5::utility::formatString("PIN:%u", pin);
        SCOPED_TRACE(s);

        uint16_t v8{}, v12{};
        EXPECT_TRUE(unit->readAnalogInput8(v8, pin));
        EXPECT_LE(v8, +UnitExtIO2::MAX_ANALOG_8);
        EXPECT_TRUE(unit->readAnalogInput12(v12, pin));
        EXPECT_LE(v12, +UnitExtIO2::MAX_ANALOG_12);
    }

    // Wrong mode
    EXPECT_TRUE(unit->writeMode(0, Mode::DigitalInput));
    uint16_t dummy{};
    EXPECT_FALSE(unit->readAnalogInput8(dummy, 0));
    EXPECT_FALSE(unit->readAnalogInput12(dummy, 0));
}

TEST_F(TestExtIO2, SinglePinServo)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->writeAllMode(Mode::ServoControl));

    for (uint8_t pin = 0; pin < UnitExtIO2::NUMBER_OF_PINS; ++pin) {
        auto s = m5::utility::formatString("PIN:%u", pin);
        SCOPED_TRACE(s);

        uint8_t deg = random_servo_angle();
        EXPECT_TRUE(unit->writeServoAngle(pin, deg));
        uint8_t rd{};
        EXPECT_TRUE(unit->readServoAngle(rd, pin));
        EXPECT_EQ(rd, deg);

        uint16_t pls = random_servo_pulse();
        EXPECT_TRUE(unit->writeServoPulse(pin, pls));
        uint16_t rp{};
        EXPECT_TRUE(unit->readServoPulse(rp, pin));
        EXPECT_EQ(rp, (uint16_t)(pls / 10 * 10));
    }

    // Wrong mode
    EXPECT_TRUE(unit->writeMode(0, Mode::DigitalInput));
    EXPECT_FALSE(unit->writeServoAngle(0, 90));
    EXPECT_FALSE(unit->writeServoPulse(0, 1500));
    uint8_t da{};
    uint16_t dp{};
    EXPECT_FALSE(unit->readServoAngle(da, 0));
    EXPECT_FALSE(unit->readServoPulse(dp, 0));
}

TEST_F(TestExtIO2, SinglePinLED)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->writeAllMode(Mode::LEDControl));

    for (uint8_t pin = 0; pin < UnitExtIO2::NUMBER_OF_PINS; ++pin) {
        auto s = m5::utility::formatString("PIN:%u", pin);
        SCOPED_TRACE(s);

        // rgb888 overload
        uint32_t color = esp_random() & 0xFFFFFF;
        EXPECT_TRUE(unit->writeLEDColor(pin, color));
        uint32_t rc{};
        EXPECT_TRUE(unit->readLEDColor(rc, pin));
        EXPECT_EQ(rc, color) << m5::utility::formatString("wrote:0x%06X read:0x%06X", color, rc);

        // r, g, b overload
        uint8_t r = esp_random() & 0xFF;
        uint8_t g = esp_random() & 0xFF;
        uint8_t b = esp_random() & 0xFF;
        EXPECT_TRUE(unit->writeLEDColor(pin, r, g, b));
        EXPECT_TRUE(unit->readLEDColor(rc, pin));
        uint32_t expected = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
        EXPECT_EQ(rc, expected) << m5::utility::formatString("wrote:0x%06X read:0x%06X", expected, rc);
    }

    // Wrong mode
    EXPECT_TRUE(unit->writeMode(0, Mode::DigitalInput));
    uint32_t dc{};
    EXPECT_FALSE(unit->readLEDColor(dc, 0));
    EXPECT_FALSE(unit->writeLEDColor(0, 0xFF0000U));
    EXPECT_FALSE(unit->writeLEDColor(0, 0xFF, 0x00, 0x00));
}

TEST_F(TestExtIO2, PinBitsLEDColorRGB)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->writeAllMode(Mode::LEDControl));

    uint8_t r         = esp_random() & 0xFF;
    uint8_t g         = esp_random() & 0xFF;
    uint8_t b         = esp_random() & 0xFF;
    uint32_t expected = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;

    EXPECT_TRUE(unit->writePinBitsLEDColor(0xFF, r, g, b));
    std::array<uint32_t, UnitExtIO2::NUMBER_OF_PINS> colors{};
    EXPECT_TRUE(unit->readPinBitsLEDColor(colors.data(), 0xFF));
    EXPECT_TRUE(check_values_eq(colors, (uint8_t)0xFF, expected)) << colors;

    EXPECT_TRUE(unit->writeAllLEDColor(r, g, b));
    EXPECT_TRUE(unit->readAllLEDColor(colors.data()));
    EXPECT_TRUE(check_values_eq(colors, (uint8_t)0xFF, expected)) << colors;
}

/*
  WARNING!!
  Failure of this test will result in an unexpected I2C address being set!
*/
TEST_F(TestExtIO2, I2CAddress)
{
    SCOPED_TRACE(ustr);

    uint8_t ver{}, addr{};

    EXPECT_FALSE(unit->changeI2CAddress(0x07));  // Invalid
    EXPECT_FALSE(unit->changeI2CAddress(0x78));  // Invalid
    EXPECT_FALSE(unit->changeI2CAddress(128));   // Invalid

    // Change to 0x09
    EXPECT_TRUE(unit->changeI2CAddress(0x09));
    EXPECT_TRUE(unit->readI2CAddress(addr));
    EXPECT_EQ(addr, 0x09);
    EXPECT_EQ(unit->address(), 0x09);

    EXPECT_TRUE(unit->readFirmwareVersion(ver));
    EXPECT_NE(ver, 0x00);

    // Change to 0x77
    EXPECT_TRUE(unit->changeI2CAddress(0x77));
    EXPECT_TRUE(unit->readI2CAddress(addr));
    EXPECT_EQ(addr, 0x77);
    EXPECT_EQ(unit->address(), 0x77);

    EXPECT_TRUE(unit->readFirmwareVersion(ver));
    EXPECT_NE(ver, 0x00);

    // Change to 0x52
    EXPECT_TRUE(unit->changeI2CAddress(0x52));
    EXPECT_TRUE(unit->readI2CAddress(addr));
    EXPECT_EQ(addr, 0x52);
    EXPECT_EQ(unit->address(), 0x52);

    EXPECT_TRUE(unit->readFirmwareVersion(ver));
    EXPECT_NE(ver, 0x00);

    // Change to default
    EXPECT_TRUE(unit->changeI2CAddress(UnitExtIO2::DEFAULT_ADDRESS));
    EXPECT_TRUE(unit->readI2CAddress(addr));
    EXPECT_EQ(addr, +UnitExtIO2::DEFAULT_ADDRESS);
    EXPECT_EQ(unit->address(), +UnitExtIO2::DEFAULT_ADDRESS);

    EXPECT_TRUE(unit->readFirmwareVersion(ver));
    EXPECT_NE(ver, 0x00);
}
