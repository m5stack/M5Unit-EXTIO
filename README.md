# M5Unit - EXTIO

## Overview

Library for EXTIO using [M5UnitUnified](https://github.com/m5stack/M5UnitUnified).  
M5UnitUnified is a library for unified handling of various M5 units products.

### SKU:U011-B

EXT.IO2 is an IO extended unit, based on STM32F030 main controller, using I2C communication interface and providing 8 IO expansion. Each IO supports independent configuration of digital I/O, ADC, SERVO control, RGB LED control modes. Supports configuration of device I2C address, which means that users can mount multiple EXT.IO2 UNITs on the same I2C BUS to extend more IO resources. Suitable for multiple digital/analog signal acquisition, with lighting/servo control applications.


## Future Plans
- EXTIO2 firmware V3 function (PWM control) is not yet supported.  
It will be supported in the next update.
- Unit EXT.IO(SKU:U011) will be supported soon.


## Related Link
See also examples using conventional methods here.

- [Unit EXT.IO2 & Datasheet](https://docs.m5stack.com/en/unit/extio2)

### Required Libraries:
- [M5UnitUnified](https://github.com/m5stack/M5UnitUnified)
- [M5Utility](https://github.com/m5stack/M5Utility)
- [M5HAL](https://github.com/m5stack/M5HAL)

## License

- [M5Unit-EXTIO - MIT](LICENSE)

## Examples
See also [examples/UnitUnified](examples/UnitUnified)

## Firmware for SKU:U011-B
See also https://github.com/m5stack/M5Unit-EXTIO2-Internal-FW

### Doxygen document
[GitHub Pages](https://m5stack.github.io/M5Unit-EXTIO/)

If you want to generate documents on your local machine, execute the following command

```
bash docs/doxy.sh
```

It will output it under docs/html  
If you want to output Git commit hashes to html, do it for the git cloned folder.

#### Required
- [Doxygen](https://www.doxygen.nl/)
- [pcregrep](https://formulae.brew.sh/formula/pcre2)
- [Git](https://git-scm.com/) (Output commit hash to html)

