# Vendored dependencies

BeanBot includes these library implementations directly so that the component
headers resolve to a specific project-local version. Versions below come from
each directory's `library.properties`.

| Directory | Library/version | BeanBot component | Purpose |
| --- | --- | --- | --- |
| `Adafruit_PWMServoDriver/` | Adafruit PWM Servo Driver Library 2.4.1 | `ServoMechanisms` | Configures the PCA9685 oscillator/PWM frequency and writes servo channel pulses over Wire |
| `HX711/` | HX711 Arduino Library 0.7.5, bogde | `Scale` | Reads the load-cell ADC, applies the scale factor and performs startup tare |
| `LiquidCrystal/` | LiquidCrystal 1.0.7 | `StockDisplay` | Drives the character LCD through its parallel four-bit interface |

## Include and build layout

The component headers under `src/beanbot/` use these exact paths:

```cpp
#include "../vendor/Adafruit_PWMServoDriver/Adafruit_PWMServoDriver.h"
#include "../vendor/HX711/HX711.h"
#include "../vendor/LiquidCrystal/LiquidCrystal.h"
```

Each library's `.cpp` includes its neighboring header. Arduino compiles these
sources recursively beneath the sketch's `src/` directory. `library.properties`
is retained as metadata; these directories are not separate Library Manager
installations. Keep them under `src/vendor/` when moving the sketch.

Each directory contains its implementation `.h`/`.cpp` pair, version metadata and
license/readme notices. Examples, development tooling and other upstream package
material are not included. The third-party implementation files are unchanged.

The Arduino board environment supplies the Arduino core and **Wire**. This
Adafruit version uses Wire directly and does not require Adafruit BusIO. Only
one HX711 implementation is included, avoiding competing `HX711.h` definitions.

## Licenses

- Adafruit PWM Servo Driver: BSD license in
  [license.txt](Adafruit_PWMServoDriver/license.txt), with accompanying attribution
  in its [README](Adafruit_PWMServoDriver/README.md).
- HX711: MIT license in [LICENSE](HX711/LICENSE).
- LiquidCrystal: GNU Lesser General Public License, version 2.1 or later, as
  stated in [README.adoc](LiquidCrystal/README.adoc).

Preserve these notices with the library files. Library metadata identifies the
upstream projects and authors.
