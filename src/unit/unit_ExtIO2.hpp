/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_ExtIO2.hpp
  @brief ExtIO2 Unit for M5UnitUnified
*/
#ifndef M5_UNIT_EXTIO_UNIT_EXTIO2_HPP
#define M5_UNIT_EXTIO_UNIT_EXTIO2_HPP

#include <M5UnitComponent.hpp>
#include <m5_utility/container/circular_buffer.hpp>
#include <array>
#include <algorithm>

namespace m5 {
namespace unit {

/*!
  @namespace extio2
  @brief For ExtIO2
 */
namespace extio2 {

/*!
  @enum Mode
  @brief Operating mode
 */
enum class Mode : int8_t {
    Invalid = -1,
    DigitalInput,   //!< Digital input
    DigitalOutput,  //!< Digital output
    ADCInput,       //!< ADC input
    ServoControl,   //!< Servo control
    LEDControl,     //!< LED control
    //    PWMControl,     //!< PWM Control (Firmware V3 or later)
};
static_assert(sizeof(Mode) == sizeof(uint8_t), "Mode and uint8_t size must match");

/*!
  @enum AnalogMode
  @brief Data width at analog input acquisition
 */
enum AnalogMode : uint8_t {
    Bits8,   //!<  8 bits (0 - 255)
    Bits12,  //!< 12 bits (0 - 4095)
};

}  // namespace extio2

/*!
  @class UnitExtIO2
  @brief Extend I/O Unit2
*/
class UnitExtIO2 : public Component {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitExtIO2, 0x45);

public:
    static constexpr uint8_t NUMBER_OF_PINS{8};  //!< The number of pins

    ///@name Valid range
    ///@{
    static constexpr uint8_t MIN_ANALOG_8{0};         //!< Minimum input of analog 8 bits mode
    static constexpr uint8_t MAX_ANALOG_8{255};       //!< Maximum input of analog 8 bits mode
    static constexpr uint16_t MIN_ANALOG_12{0};       //!< Minimum input of analog 12 bits mode
    static constexpr uint16_t MAX_ANALOG_12{4095};    //!< Maximum input of analog 12 bits mode
    static constexpr uint8_t MIN_SERVO_ANGLE{0};      //!< Minimum servo angle (degree)
    static constexpr uint8_t MAX_SERVO_ANGLE{180};    //!< Maximum servo angle (degree)
    static constexpr uint16_t MIN_SERVO_PULSE{500};   //!< Minimum servo pulse
    static constexpr uint16_t MAX_SERVO_PULSE{2500};  //!< Maximum servo pulse
    ///@}

    /*!
      @struct config_t
      @brief Settings for begin
     */
    struct config_t {
        //! Set mode on begin if true
        bool apply_mode{true};
        //! Set mode to all pins on begin if apply_mode is true
        extio2::Mode mode[NUMBER_OF_PINS] = {extio2::Mode::DigitalInput, extio2::Mode::DigitalInput,
                                             extio2::Mode::DigitalInput, extio2::Mode::DigitalInput,
                                             extio2::Mode::DigitalInput, extio2::Mode::DigitalInput,
                                             extio2::Mode::DigitalInput, extio2::Mode::DigitalInput};
    };

    //! @brief Constructor
    //! @param addr I2C address (default DEFAULT_ADDRESS)
    explicit UnitExtIO2(const uint8_t addr = DEFAULT_ADDRESS) : Component(addr)
    {
        auto ccfg  = component_config();
        ccfg.clock = 100 * 1000U;
        component_config(ccfg);
        std::fill(_mode.begin(), _mode.end(), extio2::Mode::Invalid);
    }
    //! @brief Destructor
    virtual ~UnitExtIO2()
    {
    }

    //! @brief Begin the unit
    virtual bool begin() override;

    ///@name Settings for begin
    ///@{
    /*! @brief Gets the configuration */
    inline config_t config()
    {
        return _cfg;
    }
    //! @brief Set the configuration
    inline void config(const config_t& cfg)
    {
        _cfg = cfg;
    }
    ///@}

    ///@name Firmware version
    ///@{
    /*!
      @brief Gets the inner firmware version
      @return Version
      @warning begin Cannot obtain a valid value until after success
     */
    uint8_t firmwareVersion() const
    {
        return _fw_version;
    }
    /*!
      @brief Read the firmware version
      @param[out] version Version
      @return True if successful
    */
    bool readFirmwareVersion(uint8_t& version);
    ///@}

    ///@name Mode
    ///@{
    /*!
      @brief Gets the inner mode
      @param pin Pin number
      @return Mode
    */
    inline extio2::Mode mode(const uint8_t pin)
    {
        return (pin < NUMBER_OF_PINS) ? _mode[pin] : extio2::Mode::Invalid;
    }
    /*!
      @brief Read the mode of the specified pin
      @param[out] mode Mode
      @param pin Pin number
      @return True if successful
     */
    bool readMode(extio2::Mode& mode, const uint8_t pin);
    /*!
      @brief Read the mode of all pins
      @param[out] mode Mode array of all pins
      @return True if successful
     */
    bool readAllMode(extio2::Mode mode[NUMBER_OF_PINS]);
    /*!
      @brief Write the mode of the specified pin
      @param pin Pin number
      @param mode Mode
      @return True if successful
     */
    bool writeMode(const uint8_t pin, const extio2::Mode mode);
    /*!
      @brief Write the mode of the specified pin bits
      @param pin_bits Bits of the target pin
      @param mode Mode
      @return True if successful
     */
    bool writePinBitsMode(const uint8_t pin_bits, const extio2::Mode mode);
    /*!
      @brief Write the mode of all pins
      @param mode Mode
      @return True if successful
      @note Apply mode to all pins
     */
    bool writeAllMode(const extio2::Mode mode);
    /*!
      @brief Write the mode of all pins
      @param mode Mode array
      @return True if successful
      @note Set Mode array
     */
    bool writeAllMode(const extio2::Mode mode[NUMBER_OF_PINS]);
    ///@}

    ///@name Digital input
    ///@{
    /*!
      @brief Read the digital input from the specified pin
      @param[out] high True if HIGH
      @param pin Pin number
      @return True if successful
      @pre The mode of the specified pin must be Mode::DigitalInput
     */
    bool readDigitalInput(bool& high, const uint8_t pin);
    /*!
      @brief Read the digital input from the specified pin bits
      @param[out] high_bits  Pin status 0-7 as bits of uint8_t
      @param pin_bits Bits of the target pin
      @return True if successful
      @pre The mode of the specified pin must be Mode::DigitalInput
      @warning high_bits values for unspecified pins are undefined
     */
    bool readPinBitsDigitalInput(uint8_t& high_bits, const uint8_t pin_bits);
    /*!
      @brief Read the digital input from all pins
      @param[out] high_bits  Pin status 0-7 as bits of uint8_t
      @return True if successful
      @pre The mode of all pins must be Mode::DigitalInput
     */
    bool readAllDigitalInput(uint8_t& high_bits);
    ///@}

    ///@name Digital output
    ///@{
    /*!
      @brief Write the digital output to the specified pin
      @param pin Pin number
      @param high HIGH if true, LOW if false
      @return True if successful
      @pre The mode of the specified pin must be Mode::DigitalOutput
     */
    bool writeDigitalOutput(const uint8_t pin, const bool high);
    /*!
      @brief Write the digital output HIGH to the specified pin
      @param pin Pin number
      @return True if successful
      @pre The mode of the specified pin must be Mode::DigitalOutput
     */
    inline bool writeDigitalOutputHigh(const uint8_t pin)
    {
        return writeDigitalOutput(pin, true);
    }
    /*!
      @brief Write the digital output LOW to the specified pin
      @param pin Pin number
      @return True if successful
      @pre The mode of the specified pin must be Mode::DigitalOutput
     */
    inline bool writeDigitalOutputLow(const uint8_t pin)
    {
        return writeDigitalOutput(pin, false);
    }
    /*!
      @brief Write the digital output to the specified pin bits
      @param pin_bits Bits of the target pin
      @param high HIGH if true, LOW if false
      @return True if successful
      @pre The mode of the specified pin must be Mode::DigitalOutput
     */
    template <typename T, typename std::enable_if<std::is_same<T, bool>::value, std::nullptr_t>::type = nullptr>
    inline bool writePinBitsDigitalOutput(const uint8_t pin_bits, const T high)
    {
        return write_pin_bits_digital_output(pin_bits, high);
    }
    /*!
      @brief Write the digital output to the specified pin bits
      @param pin_bits Bits of the target pin
      @param high_bits Pin bits to be set HIGH
      @return True if successful
      @pre The mode of the specified pin must be Mode::DigitalOutput
      @note The value of high_bits for unspecified pin bits is ignored
     */
    template <typename T, typename std::enable_if<!std::is_same<T, bool>::value && std::is_integral<T>::value,
                                                  std::nullptr_t>::type = nullptr>
    inline bool writePinBitsDigitalOutput(const uint8_t pin_bits, const T high_bits)
    {
        return write_pin_bits_digital_output(pin_bits, static_cast<uint8_t>(high_bits));
    }
    /*!
      @brief Write the digital output HIGH to the specified pin bits
      @param pin_bits Bits of the target pin
      @return True if successful
      @pre The mode of the specified pin must be Mode::DigitalOutput
     */
    inline bool writePinBitsDigitalOutputHigh(const uint8_t pin_bits)
    {
        return writePinBitsDigitalOutput(pin_bits, true);
    }
    /*!
      @brief Write the digital output LOW to the specified pin bits
      @param pin_bits Bits of the target pin
      @return True if successful
      @pre The mode of the specified pin must be Mode::DigitalOutput
     */
    inline bool writePinBitsDigitalOutputLow(const uint8_t pin_bits)
    {
        return writePinBitsDigitalOutput(pin_bits, false);
    }
    /*!
      @brief Write the digital output to all pins
      @param high HIGH if true, LOW if false
      @return True if successful
      @pre The mode of all pins must be Mode::DigitalOutput
     */
    inline bool writeAllDigitalOutput(const bool high)
    {
        return writePinBitsDigitalOutput(0xFF, high);
    }
    /*!
      @brief Write the digital output HIGH to all pins
      @return True if successful
      @pre The mode of all pins must be Mode::DigitalOutput
     */
    inline bool writeAllDigitalOutputHigh()
    {
        return writeAllDigitalOutput(true);
    }
    /*!
      @brief Write the digital output LOW to all pins
      @return True if successful
      @pre The mode of all pins must be Mode::DigitalOutput
     */
    inline bool writeAllDigitalOutputLow()
    {
        return writeAllDigitalOutput(false);
    }
    ///@}

    ///@note Analog 8 bits range 0 - 255
    ///@note Analog 12 bits range 0 - 4095
    ///@name Analog input
    ///@{
    /*!
      @brief Read the analog input from the specified pin
      @param[out] value Value
      @param pin Pin number
      @param amode Data width
      @return True if successful
      @pre The mode of the specified pin must be Mode::ADCInput
     */
    bool readAnalogInput(uint16_t& value, const uint8_t pin, const extio2::AnalogMode amode);
    //! @brief Read the analog input 8 bits from the specified pin
    inline bool readAnalogInput8(uint16_t& value, const uint8_t pin)
    {
        return readAnalogInput(value, pin, extio2::AnalogMode::Bits8);
    }
    //! @brief Read the analog input 12 bits from the specified pin
    inline bool readAnalogInput12(uint16_t& value, const uint8_t pin)
    {
        return readAnalogInput(value, pin, extio2::AnalogMode::Bits12);
    }
    /*!
      @brief Read the analog input from the specified pin bits
      @param[out] values Value array
      @param pin_bits Bits of the target pin
      @param amode Data width
      @return True if successful
      @pre The mode of the specified pin must be Mode::ADCInput
      @warning values values for unspecified pins are undefined
     */
    bool readPinBitsAnalogInput(uint16_t values[NUMBER_OF_PINS], const uint8_t pin_bits,
                                const extio2::AnalogMode amode);
    //! @brief Read the analog input 8 bits from the specified pin bits
    inline bool readPinBitsAnalogInput8(uint16_t values[NUMBER_OF_PINS], const uint8_t pin_bits)
    {
        return readPinBitsAnalogInput(values, pin_bits, extio2::AnalogMode::Bits8);
    }
    //! @brief Read the analog input 12 bits from the specified pin bits
    inline bool readPinBitsAnalogInput12(uint16_t values[NUMBER_OF_PINS], const uint8_t pin_bits)
    {
        return readPinBitsAnalogInput(values, pin_bits, extio2::AnalogMode::Bits12);
    }
    /*!
      @brief Read the analog input from all pins
      @param[out] values Value array
      @param amode Data width
      @return True if successful
      @pre The mode of all pins must be Mode::ADCInput
     */
    inline bool readAllAnalogInput(uint16_t values[NUMBER_OF_PINS], const extio2::AnalogMode amode)
    {
        return readPinBitsAnalogInput(values, 0xFF, amode);
    }
    //! @brief Read the analog input 8 bits from all pins
    bool readAllAnalogInput8(uint16_t values[NUMBER_OF_PINS])
    {
        return readAllAnalogInput(values, extio2::AnalogMode::Bits8);
    }
    //! @brief Read the analog input 12 bits from all pins
    bool readAllAnalogInput12(uint16_t values[NUMBER_OF_PINS])
    {
        return readAllAnalogInput(values, extio2::AnalogMode::Bits12);
    }
    ///@}

    ///@note Valid angle degree range 0 - 180
    ///@note Valid pulse range 500 - 2500
    ///@name Servo control
    ///@{
    /*!
      @brief Read the servo angle from the specified pin
      @param[out] degree Angle
      @param pin Pin number
      @return True if successful
      @pre The mode of the specified pin must be Mode::ServoControl
     */
    bool readServoAngle(uint8_t& degree, const uint8_t pin);
    /*!
      @brief Read the servo angle from the specified pin bits
      @param[out] degrees Angle array of all pins
      @param pin_bits Bits of the target pin
      @return True if successful
      @pre The mode of the specified pin must be Mode::ServoControl
      @warning degrees values for unspecified pins are undefined
     */
    bool readPinBitsServoAngle(uint8_t degrees[NUMBER_OF_PINS], const uint8_t pin_bits);
    /*!
      @brief Read the servo angle from all pins
      @param[out] degrees Angle array of all pins
      @return True if successful
      @pre The mode of all pins must be Mode::ServoControl
     */
    inline bool readAllServoAngle(uint8_t degrees[NUMBER_OF_PINS])
    {
        return readPinBitsServoAngle(degrees, 0xFF);
    }
    /*!
      @brief Write the servo angle to the specified pin
      @param pin Pin number
      @param degree Angle
      @return True if successful
      @pre The mode of the specified pin must be Mode::ServoControl
     */
    bool writeServoAngle(const uint8_t pin, const uint8_t degree);
    /*!
      @brief Write the servo angle to the specified pin bits
      @param pin_bits Bits of the target pin
      @param degree Angle
      @return True if successful
      @pre The mode of the specified pin must be Mode::ServoControl
     */
    bool writePinBitsServoAngle(const uint8_t pin_bits, const uint8_t degree);
    /*!
      @brief Write the servo angle to all pins
      @param degree Angle
      @return True if successful
      @pre The mode of all pins must be Mode::ServoControl
     */
    inline bool writeAllServoAngle(const uint8_t degree)
    {
        return writePinBitsServoAngle(0xFF, degree);
    }
    /*!
      @brief Read the servo pulse from the specified pin
      @param[out] pulse Pulse (us)
      @param pin Pin number
      @return True if successful
      @pre The mode of the specified pin must be Mode::ServoControl
     */
    bool readServoPulse(uint16_t& pulse, const uint8_t pin);
    /*!
      @brief Read the servo pulse from the specified pin bits
      @param[out] pulses Pulse array
      @param pin_bits Bits of the target pin
      @return True if successful
      @pre The mode of the specified pin must be Mode::ServoControl
      @warning pulses values for unspecified pins are undefined
     */
    bool readPinBitsServoPulse(uint16_t pulses[NUMBER_OF_PINS], const uint8_t pin_bits);
    /*!
      @brief Read the servo pulse from all pins
      @param[out] pulses Pulse array
      @return True if successful
      @pre The mode of all pins must be Mode::ServoControl
     */
    inline bool readAllServoPulse(uint16_t pulses[NUMBER_OF_PINS])
    {
        return readPinBitsServoPulse(pulses, 0xFF);
    }
    /*!
      @brief Write the servo pulse to the specified pin
      @param pin Pin number
      @param pulse Pulse
      @return True if successful
      @pre The mode of the specified pin must be Mode::ServoControl
     */
    bool writeServoPulse(const uint8_t pin, const uint16_t pulse);
    /*!
      @brief Write the servo pulse to the specified pin bits
      @param pin_bits Bits of the target pin
      @param pulse Pulse
      @return True if successful
      @pre The mode of the specified pin must be Mode::ServoControl
     */
    bool writePinBitsServoPulse(const uint8_t pin_bits, const uint16_t pulse);
    /*!
      @brief Write the servo pulse to all pins
      @param pulse Pulse
      @return True if successful
      @pre The mode of all pins must be Mode::ServoControl
     */
    inline bool writeAllServoPulse(const uint16_t pulse)
    {
        return writePinBitsServoPulse(0xFF, pulse);
    }
    ///@}

    ///@name LED control
    ///@{
    /*!
      @brief Read the LED RGB888 from the specified pin
      @param[out] rgb888 RGB888
      @param pin Pin number
      @return True if successful
      @pre The mode of the specified pin must be Mode::LEDControl
     */
    bool readLEDColor(uint32_t& rgb888, const uint8_t pin);
    /*!
      @brief Read the LED RGB888 from the specified pin bits
      @param[out] rgb888 RGB888 array
      @param pin_bits Bits of the target pin
      @return True if successful
      @pre The mode of the specified pin must be Mode::LEDControl
      @warning rgb888 values for unspecified pins are undefined
     */
    bool readPinBitsLEDColor(uint32_t rgb888[NUMBER_OF_PINS], const uint8_t pin_bits);
    /*!
      @brief Read the LED RGB888 from all pins
      @param[out] rgb888 RGB888 array
      @return True if successful
      @pre The mode of all pins must be Mode::LEDControl
     */
    inline bool readAllLEDColor(uint32_t rgb888[NUMBER_OF_PINS])
    {
        return readPinBitsLEDColor(rgb888, 0xFF);
    }
    /*!
      @brief Write the LED RGB888 to the specified pin
      @param pin Pin number
      @param rgb888 RGB888
      @return True if successful
      @pre The mode of the specified pin must be Mode::LEDControl
     */
    inline bool writeLEDColor(const uint8_t pin, const uint32_t rgb888)
    {
        return writeLEDColor(pin, (rgb888 >> 16) & 0xFF, (rgb888 >> 8) & 0xFF, rgb888 & 0xFF);
    }
    /*!
      @brief Write the LED RGB888 to the specified pin
      @param pin Pin number
      @param r Red
      @param g Green
      @param b Blue
      @return True if successful
      @pre The mode of the specified pin must be Mode::LEDControl
     */
    bool writeLEDColor(const uint8_t pin, const uint8_t r, const uint8_t g, const uint8_t b);
    /*!
      @brief Write the LED RGB888 to the specified pin bits
      @param pin_bits Bits of the target pin
      @param rgb888 RGB888
      @return True if successful
      @pre The mode of the specified pin must be Mode::LEDControl
     */
    inline bool writePinBitsLEDColor(const uint8_t pin_bits, const uint32_t rgb888)
    {
        return writePinBitsLEDColor(pin_bits, (rgb888 >> 16) & 0xFF, (rgb888 >> 8) & 0xFF, rgb888 & 0xFF);
    }
    /*!
      @brief Write the LED RGB888 to the specified pin bits
      @param pin_bits Bits of the target pin
      @param r Red
      @param g Green
      @param b Blue
      @return True if successful
      @pre The mode of the specified pin must be Mode::LEDControl
     */
    bool writePinBitsLEDColor(const uint8_t pin_bits, const uint8_t r, const uint8_t g, const uint8_t b);
    /*!
      @brief Write the LED RGB888 to all pins
      @param rgb888 RGB888
      @return True if successful
      @pre The mode of all pins must be Mode::LEDControl
     */
    inline bool writeAllLEDColor(const uint32_t rgb888)
    {
        return writePinBitsLEDColor(0xFF, rgb888);
    }
    /*!
      @brief Write the LED RGB888 to all pins
      @param r Red
      @param g Green
      @param b Blue
      @return True if successful
      @pre The mode of all pins must be Mode::LEDControl
     */
    inline bool writeAllLEDColor(const uint8_t r, const uint8_t g, const uint8_t b)
    {
        return writePinBitsLEDColor(0xFF, r, g, b);
    }
    ///@}

    ///@warning Handling warning
    ///@warning Repeated writing may cause partition damage
    ///@name I2C Address
    ///@{
    /*!
      @brief Change device I2C address
      @param i2c_address I2C address
      @return True if successful
      @warning Do not change the I2C address configuration repeatedly at high frequency
    */
    bool changeI2CAddress(const uint8_t i2c_address);
    /*!
      @brief Read device I2C address
      @param[out] i2c_address I2C address
      @return True if successful
    */
    bool readI2CAddress(uint8_t& i2c_address);
    ///@}

protected:
    bool write_pin_bits_digital_output(const uint8_t pin_bits, const bool high);
    bool write_pin_bits_digital_output(const uint8_t pin_bits, const uint8_t high_bits);

    static constexpr uint8_t FIRMWARE_VERSION_CAN_PWM_CONTROL{0x03};
    inline bool canPWMControl() const
    {
        return _fw_version >= FIRMWARE_VERSION_CAN_PWM_CONTROL;
    }

private:
    std::array<extio2::Mode, NUMBER_OF_PINS> _mode{};
    config_t _cfg{};
    uint8_t _fw_version{};
};

///@cond
namespace extio2 {
namespace command {
constexpr uint8_t MODE_REG{0x00};
constexpr uint8_t OUTPUT_CTL_REG{0x10};
// constexpr uint8_t OUTPUTS_CTL_REG{0x18};
constexpr uint8_t DIGITAL_INPUT_REG{0x20};
// constexpr uint8_t DIGITAL_INPUTS_REG{0x28};
constexpr uint8_t ANALOG_INPUT_8BITS_REG{0x30};
constexpr uint8_t ANALOG_INPUT_12BITS_REG{0x40};
constexpr uint8_t SERVO_ANGLE_8BITS_REG{0x50};
constexpr uint8_t SERVO_PULSE_16BITS_REG{0x60};
constexpr uint8_t RGB_24BITS_REG{0x70};
constexpr uint8_t FW_VERSION_REG{0xFE};
constexpr uint8_t ADDRESS_REG{0xFF};
constexpr uint8_t PWM_DUTY_REG{0x90};  // Firmware V3 or later
constexpr uint8_t PWM_FREQ_REG{0xA0};  // Firmware V3 or later

}  // namespace command
}  // namespace extio2
///@endcond

}  // namespace unit
}  // namespace m5
#endif
