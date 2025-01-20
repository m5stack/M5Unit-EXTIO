/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_ExtIO2.cpp
  @brief ExtIO2 Unit for M5UnitUnified
*/
#include "unit_ExtIO2.hpp"
#include <M5Utility.hpp>
#include <algorithm>

using namespace m5::utility::mmh3;
using namespace m5::unit::types;
using namespace m5::unit::extio2;
using namespace m5::unit::extio2::command;

namespace {
inline bool is_valid_mode(Mode m)
{
    return (m5::stl::to_underlying(m) >= 0) && (m5::stl::to_underlying(m) <= m5::stl::to_underlying(Mode::LEDControl));
}

constexpr uint8_t analog_input_reg_table[] = {
    ANALOG_INPUT_8BITS_REG,
    ANALOG_INPUT_12BITS_REG,
};

}  // namespace

namespace m5 {
namespace unit {

// class UnitExtIO2
const char UnitExtIO2::name[] = "UnitExtIO2";
const types::uid_t UnitExtIO2::uid{"UnitExtIO2"_mmh3};
const types::uid_t UnitExtIO2::attr{0};

bool UnitExtIO2::begin()
{
    for (uint8_t a = 0; a < 200; ++a) {
        Wire.beginTransmission(a);
        if (Wire.endTransmission() == 0) {
            M5_LIB_LOGE("[%02X]:Detected", a);
        }
    }

    if (!readFirmwareVersion(_fw_version) || _fw_version == 0x00) {
        M5_LIB_LOGE("Cannot detect %s. Addr:%X FW:%X", deviceName(), address(), _fw_version);
        return false;
    }

    if (!readAllMode(_mode.data())) {
        M5_LIB_LOGE("Failed to readAllMode");
        return false;
    }
    return _cfg.apply_mode ? writeAllMode(_cfg.mode) : true;
}

bool UnitExtIO2::readFirmwareVersion(uint8_t& version)
{
    return readRegister8(FW_VERSION_REG, version, 0);
}

bool UnitExtIO2::readMode(extio2::Mode& mode, const uint8_t pin)
{
    if (pin >= NUMBER_OF_PINS) {
        M5_LIB_LOGE("Invalid pin number %u", pin);
        return false;
    }

    uint8_t v{};
    if (readRegister8((uint8_t)(MODE_REG + pin), v, 0)) {
        mode = static_cast<Mode>(v);
        return true;
    }
    return false;
}

bool UnitExtIO2::readAllMode(extio2::Mode mode[NUMBER_OF_PINS])
{
    if (mode) {
        std::fill(mode, mode + NUMBER_OF_PINS, Mode::Invalid);

        // MODE_REG cannot burst read
        for (uint8_t pin = 0; pin < NUMBER_OF_PINS; ++pin) {
            if (!readMode(mode[pin], pin)) {
                return false;
            }
        }
        return true;
    }
    return false;
}

bool UnitExtIO2::writeMode(const uint8_t pin, const extio2::Mode mode)
{
    if (pin >= NUMBER_OF_PINS || !is_valid_mode(mode)) {
        M5_LIB_LOGE("Invalid parameter pin:%u mode:%d", pin, mode);
        return false;
    }
    if (writeRegister8((uint8_t)(MODE_REG + pin), m5::stl::to_underlying(mode))) {
        _mode[pin] = mode;
        return true;
    }
    return false;
}

bool UnitExtIO2::writePinBitsMode(const uint8_t pin_bits, const extio2::Mode mode)
{
    if (!is_valid_mode(mode)) {
        M5_LIB_LOGE("Invalid parameter mode:%d", mode);
        return false;
    }
    if (pin_bits == 0) {
        M5_LIB_LOGE("Target not specified");
        return false;
    }

    for (uint8_t pin = 0; pin < NUMBER_OF_PINS; ++pin) {
        if ((1U << pin) & pin_bits) {
            if (!writeMode(pin, mode)) {
                return false;
            }
        }
    }
    return true;
}

bool UnitExtIO2::writeAllMode(const extio2::Mode mode)
{
    std::array<Mode, NUMBER_OF_PINS> tmp{};
    std::fill(tmp.begin(), tmp.end(), mode);
    return writeAllMode(tmp.data());
}

bool UnitExtIO2::writeAllMode(const extio2::Mode mode[NUMBER_OF_PINS])
{
    if (mode) {
        if (std::any_of(mode, mode + NUMBER_OF_PINS, [](const Mode m) { return !is_valid_mode(m); })) {
            M5_LIB_LOGE("Invalid mode included");
            return false;
        }
        if (writeRegister(MODE_REG, (const uint8_t*)mode, NUMBER_OF_PINS)) {
            std::copy(mode, mode + NUMBER_OF_PINS, _mode.begin());
            return true;
        }
    }
    return false;
}

//
bool UnitExtIO2::readDigitalInput(bool& high, const uint8_t pin)
{
    uint8_t v{};
    high = false;
    if (mode(pin) == Mode::DigitalInput && readRegister8((uint8_t)(DIGITAL_INPUT_REG + pin), v, 0)) {
        high = v;
        return true;
    }
    return false;
}

bool UnitExtIO2::readPinBitsDigitalInput(uint8_t& high_bits, const uint8_t pin_bits)
{
    high_bits = 0;
    if (pin_bits == 0) {
        M5_LIB_LOGE("Target not specified");
        return false;
    }

    for (uint8_t pin = 0; pin < NUMBER_OF_PINS; ++pin) {
        if ((1U << pin) & pin_bits) {
            bool h{};
            if (!readDigitalInput(h, pin)) {
                return false;
            }
            high_bits |= (uint8_t)h << pin;
        }
    }
    return true;
}

bool UnitExtIO2::readAllDigitalInput(uint8_t& high_bits)
{
    high_bits = 0;
#if 0
    return std::all_of(_mode.begin(), _mode.end(), [](const Mode m) { return m == Mode::DigitalInput; }) &&
           readRegister8(DIGITAL_INPUTS_REG, high_bits, 0);
#else
    return readPinBitsDigitalInput(high_bits, 0xFF);
#endif
}

//
bool UnitExtIO2::writeDigitalOutput(const uint8_t pin, const bool high)
{
    return (mode(pin) == Mode::DigitalOutput) && writeRegister8((uint8_t)(OUTPUT_CTL_REG + pin), high);
}

bool UnitExtIO2::write_pin_bits_digital_output(const uint8_t pin_bits, const bool high)
{
    if (pin_bits == 0) {
        M5_LIB_LOGE("Target not specified");
        return false;
    }

    for (uint8_t pin = 0; pin < NUMBER_OF_PINS; ++pin) {
        if ((1U << pin) & pin_bits) {
            if (!writeDigitalOutput(pin, high)) {
                return false;
            }
        }
    }
    return true;
}

bool UnitExtIO2::write_pin_bits_digital_output(const uint8_t pin_bits, const uint8_t high_bits)
{
    if (pin_bits == 0) {
        M5_LIB_LOGE("Target not specified");
        return false;
    }

    for (uint8_t pin = 0; pin < NUMBER_OF_PINS; ++pin) {
        if ((1U << pin) & pin_bits) {
            if (!writeDigitalOutput(pin, (1U << pin) & high_bits)) {
                return false;
            }
        }
    }
    return true;
}

#if 0
bool UnitExtIO2::writeAllDigitalOutput(const bool high)
{
    return std::all_of(_mode.begin(), _mode.end(), [](const Mode m) { return m == Mode::DigitalOutput; })
               ? writeRegister8(OUTPUTS_CTL_REG, (uint8_t)high)
               : false;
}
#endif

//
bool UnitExtIO2::readAnalogInput(uint16_t& value, const uint8_t pin, const extio2::AnalogMode amode)
{
    value = 0;

    union {
        uint16_t v16;
        uint8_t v8[2];
    } buf{};
    uint8_t reg = analog_input_reg_table[m5::stl::to_underlying(amode)] + pin * (1 + m5::stl::to_underlying(amode));

    if (mode(pin) == Mode::ADCInput && readRegister(reg, buf.v8, 1 + m5::stl::to_underlying(amode), 0)) {
        value = buf.v16;
        return true;
    }
    return false;
}

bool UnitExtIO2::readPinBitsAnalogInput(uint16_t values[NUMBER_OF_PINS], const uint8_t pin_bits,
                                        const extio2::AnalogMode amode)
{
    if (pin_bits == 0) {
        M5_LIB_LOGE("Target not specified");
        return false;
    }

    if (values) {
        std::fill(values, values + NUMBER_OF_PINS, 0);

        for (uint8_t pin = 0; pin < NUMBER_OF_PINS; ++pin) {
            if ((1U << pin) & pin_bits) {
                if (!readAnalogInput(values[pin], pin, amode)) {
                    return false;
                }
            }
        }
        return true;
    }
    return false;
}

//
bool UnitExtIO2::readServoAngle(uint8_t& degree, const uint8_t pin)
{
    degree = 0;
    return mode(pin) == Mode::ServoControl && readRegister8((uint8_t)(SERVO_ANGLE_8BITS_REG + pin), degree, 0);
}

bool UnitExtIO2::readPinBitsServoAngle(uint8_t degrees[NUMBER_OF_PINS], const uint8_t pin_bits)
{
    if (pin_bits == 0) {
        M5_LIB_LOGE("Target not specified");
        return false;
    }

    if (degrees) {
        std::fill(degrees, degrees + NUMBER_OF_PINS, 0);

        for (uint8_t pin = 0; pin < NUMBER_OF_PINS; ++pin) {
            if ((1U << pin) & pin_bits) {
                if (!readServoAngle(degrees[pin], pin)) {
                    return false;
                }
            }
        }
        return true;
    }
    return false;
}

bool UnitExtIO2::writeServoAngle(const uint8_t pin, const uint8_t degree)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
    if (degree < MIN_SERVO_ANGLE || degree > MAX_SERVO_ANGLE) {
        M5_LIB_LOGE("Valid range %u - %u, %u", MIN_SERVO_ANGLE, MAX_SERVO_ANGLE, degree);
        return false;
    }
#pragma GCC diagnostic pop
    return mode(pin) == Mode::ServoControl && writeRegister8((uint8_t)(SERVO_ANGLE_8BITS_REG + pin), degree);
}

bool UnitExtIO2::writePinBitsServoAngle(const uint8_t pin_bits, const uint8_t degree)
{
    if (pin_bits == 0) {
        M5_LIB_LOGE("Target not specified");
        return false;
    }

    for (uint8_t pin = 0; pin < NUMBER_OF_PINS; ++pin) {
        if ((1U << pin) & pin_bits) {
            if (!writeServoAngle(pin, degree)) {
                return false;
            }
        }
    }
    return true;
}

bool UnitExtIO2::readServoPulse(uint16_t& pulse, const uint8_t pin)
{
    pulse = 0;
    return mode(pin) == Mode::ServoControl && readRegister16LE((uint8_t)(SERVO_PULSE_16BITS_REG + pin * 2), pulse, 0);
}

bool UnitExtIO2::readPinBitsServoPulse(uint16_t pulses[NUMBER_OF_PINS], const uint8_t pin_bits)
{
    if (pin_bits == 0) {
        M5_LIB_LOGE("Target not specified");
        return false;
    }

    if (pulses) {
        std::fill(pulses, pulses + NUMBER_OF_PINS, 0);

        for (uint8_t pin = 0; pin < NUMBER_OF_PINS; ++pin) {
            if ((1U << pin) & pin_bits) {
                if (!readServoPulse(pulses[pin], pin)) {
                    return false;
                }
            }
        }
        return true;
    }
    return false;
}

bool UnitExtIO2::writeServoPulse(const uint8_t pin, const uint16_t pulse)
{
    if (pulse < MIN_SERVO_PULSE || pulse > MAX_SERVO_PULSE) {
        M5_LIB_LOGE("Valid range %u - %u, %u", MIN_SERVO_PULSE, MAX_SERVO_PULSE, pulse);
        return false;
    }
    return mode(pin) == Mode::ServoControl && writeRegister16LE((uint8_t)(SERVO_PULSE_16BITS_REG + pin * 2), pulse);
}

bool UnitExtIO2::writePinBitsServoPulse(const uint8_t pin_bits, const uint16_t pulse)
{
    if (pin_bits == 0) {
        M5_LIB_LOGE("Target not specified");
        return false;
    }

    for (uint8_t pin = 0; pin < NUMBER_OF_PINS; ++pin) {
        if ((1U << pin) & pin_bits) {
            if (!writeServoPulse(pin, pulse)) {
                return false;
            }
        }
    }
    return true;
}

//
bool UnitExtIO2::readLEDColor(uint32_t& rgb888, const uint8_t pin)
{
    rgb888 = 0;
    uint8_t rbuf[3]{};
    if (mode(pin) == Mode::LEDControl && readRegister((uint8_t)(RGB_24BITS_REG + pin * 3), rbuf, 3, 1)) {
        rgb888 = ((uint32_t)rbuf[0] << 16) | ((uint32_t)rbuf[1] << 8) | rbuf[2];
        return true;
    }
    return false;
}

bool UnitExtIO2::readPinBitsLEDColor(uint32_t rgb888[NUMBER_OF_PINS], const uint8_t pin_bits)
{
    if (pin_bits == 0) {
        M5_LIB_LOGE("Target not specified");
        return false;
    }
    if (rgb888) {
        std::fill(rgb888, rgb888 + NUMBER_OF_PINS, 0);
        for (uint8_t pin = 0; pin < NUMBER_OF_PINS; ++pin) {
            if ((1U << pin) & pin_bits) {
                if (!readLEDColor(rgb888[pin], pin)) {
                    return false;
                }
            }
        }
        return true;
    }
    return false;
}

bool UnitExtIO2::writeLEDColor(const uint8_t pin, const uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t buf[3]{r, g, b};
    return mode(pin) == Mode::LEDControl && writeRegister((uint8_t)(RGB_24BITS_REG + pin * 3), buf, 3);
}

bool UnitExtIO2::writePinBitsLEDColor(const uint8_t pin_bits, const uint8_t r, uint8_t g, uint8_t b)
{
    if (pin_bits == 0) {
        M5_LIB_LOGE("Target not specified");
        return false;
    }

    for (uint8_t pin = 0; pin < NUMBER_OF_PINS; ++pin) {
        if ((1U << pin) & pin_bits) {
            if (!writeLEDColor(pin, r, g, b)) {
                return false;
            }
        }
    }
    return true;
}

bool UnitExtIO2::readI2CAddress(uint8_t& i2c_address)
{
    i2c_address = 0;
    return readRegister8(ADDRESS_REG, i2c_address, 1);
}

bool UnitExtIO2::changeI2CAddress(const uint8_t i2c_address)
{
    if (!m5::utility::isValidI2CAddress(i2c_address) || i2c_address > 127) {
        M5_LIB_LOGE("Invalid address : %02X", i2c_address);
        return false;
    }
    if (writeRegister8(ADDRESS_REG, i2c_address) && changeAddress(i2c_address)) {
        uint8_t v{};
        bool done{};
        auto timeout_at = m5::utility::millis() + 1000;
        do {
            m5::utility::delay(100);
            done = readFirmwareVersion(v) && (v != 0);
        } while (!done && m5::utility::millis() <= timeout_at);
        return done;
    }
    return false;
}

}  // namespace unit
}  // namespace m5
