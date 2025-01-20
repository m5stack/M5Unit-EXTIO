/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example using M5UnitUnified for UnitExtIO2
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

#if 0
struct Pin {
    explicit Pin(const uint8_t pin, const Mode m) : _pin(pin), _mode{m}
    {
    }
    inline uint8_t pin() const
    {
        return _pin;
    }
    inline Mode mode() const
    {
        return _mode;
    }
    virtual bool pump(const unsigned long at) = 0;
    inline bool updated() const
    {
        return _updated;
    }

  protected:
    bool _updated{};
    
private:
    uint8_t _pin{};
    Mode _mode{};
};

struct DigitalOutputPin : Pin {
    DigitalOutputPin(const uint8_t pin) : Pin(pin, Mode::DigiatlOutput)
    {
        latest = m5::utility::millis();
    }
    bool high() const
    {
        return _high;
    }
    virtual bool pump(const unsigned long at) override
    {
        _updated = false;
        if (at >= latest + 250) {
            _high   = !_high;
            _latest = at;
            _updated = true;
        }
        return updated();
    }

protected:
    unsigned long latest{};
    bool _high{};
};

DigitalOutputPin pin0(0);
DigitalOutputPin pin7(7);
Pin* pin_behavior[8] = {&pin0,   nullptr, nullptr, nullptr, nullptr,
                        nullptr, nullptr, &pin7
};

#endif

constexpr uint8_t do_pin_bits  = {(1U << 0) | (1U << 7)};  // target pin number bits (0 and 7)
constexpr uint8_t di_pin_bits  = {(1U << 1) | (1U << 6)};  // target pin number bits (1 and 6)
constexpr uint8_t ai_pin_bits  = {(1U << 2) | (1U << 5)};  // target pin number bits (2 and 5)
constexpr uint8_t rgb_pin_bits = {(1U << 3) | (1U << 4)};  // target pin number bits (3 and 4)

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
    lcd.clear(TFT_BLACK);

    unit.writePinBitsMode(do_pin_bits, Mode::DigitalOutput);
    unit.writePinBitsDigitalOutputLow(do_pin_bits);

    unit.writePinBitsMode(di_pin_bits, Mode::DigitalInput);
}

void loop()
{
    M5.update();
    auto touch = M5.Touch.getDetail();

#if 0    
    auto at = m5::utility::millis();
    for (uint32_t i =0; i<8;++i) {
        if(pin_behavior[i]) {
            if(p->pump(at)) {


            }
        }
    }
#endif

#if 1
    uint8_t hbits{};

    lcd.setCursor(0, 0);
    lcd.printf("HIGH");
    unit.writePinBitsDigitalOutputHigh(do_pin_bits);
    unit.readPinBitsDigitalInput(hbits, di_pin_bits);
    M5_LOGI("DI:%X", hbits);

    delay(250);

    lcd.setCursor(0, 0);
    lcd.printf("LOW ");
    unit.writePinBitsDigitalOutputLow(do_pin_bits);
    unit.readPinBitsDigitalInput(hbits, di_pin_bits);
    M5_LOGI("DI:%X", hbits);
    delay(250);

    /*

    Units.update();
    if (unit.updated()) {
    }

    if (M5.BtnA.wasClicked() || touch.wasClicked()) {
    }
    */
#endif
}
