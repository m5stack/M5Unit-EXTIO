# M5Unit - EXTIO

## Overview

Library for EXTIO using [M5UnitUnified](https://github.com/m5stack/M5UnitUnified).  
M5UnitUnified is a library for unified handling of various M5 units products.

### SKU:U011-B

Unit EXT.IO2 is an IO expansion unit developed based on the STM32F030 main controller, using an I2C communication interface, providing 8-channel IO expansion. Each IO channel supports independent configuration of digital input/output, ADC, SERVO control, and RGB LED control modes. It supports configuring the device's I2C address, which means users can mount multiple Unit EXT.IO2 units on the same I2C bus to expand more IO resources. It is suitable for applications such as multi-channel digital/analog signal acquisition and light/servo control.


## Future Plans
- EXTIO2 firmware V3 function (PWM control) is not yet supported.  
It will be supported in the next update.
- Unit EXT.IO(SKU:U011) will be supported soon.


## Related Link

- [Unit EXT.IO2 & Datasheet](https://docs.m5stack.com/en/unit/extio2)

## Required Libraries

- [M5UnitUnified](https://github.com/m5stack/M5UnitUnified)
- [M5Utility](https://github.com/m5stack/M5Utility)
- [M5HAL](https://github.com/m5stack/M5HAL)

## License

- [M5Unit-EXTIO - MIT](LICENSE)

## Examples
See also [examples/UnitUnified](examples/UnitUnified)

### For ESP-IDF settings

> **NOTE:** The ESP-IDF native build (`idf.py`) targets ESP-IDF **5.1 or later** (5.x and 6.x).

Each example covers a single unit, so the examples need no Kconfig choice. Build them directly:

```sh
cd examples/UnitUnified/UnitExtIO2/AnaDigInput
idf.py set-target esp32s3               # or esp32 / esp32c5 / esp32c6 / esp32h2 / esp32p4 / ...
idf.py build flash monitor
```

## Support via [PaHub](https://docs.m5stack.com/en/unit/Unit-PaHub%20v2.1)

|Unit|Support|Note|
|---|---|---|
|UnitExtIO2|OK||

UnitExtIO2 units share the same default address (0x45), so they can be used at the same time by connecting each to a different PaHub channel.
See also the [ViaPaHub](examples/UnitUnified/UnitExtIO2/ViaPaHub) example and [M5Unit-HUB](https://github.com/m5stack/M5Unit-HUB)

## Firmware for SKU:U011-B
See also https://github.com/m5stack/M5Unit-EXTIO2-Internal-FW

## Doxygen document
[GitHub Pages](https://m5stack.github.io/M5Unit-EXTIO/)

If you want to generate documents on your local machine, execute the following command

```
bash docs/doxy.sh
```

It will output it under docs/html  
If you want to output Git commit hashes to html, do it for the git cloned folder.

### Required
- [Doxygen](https://www.doxygen.nl/)
- [Git](https://git-scm.com/) (Output commit hash to html)

