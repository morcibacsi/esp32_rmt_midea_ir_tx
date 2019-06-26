# ESP32 RMT peripheral IR remote control library for Midea Air conditioner.

## Arduino friendly library utilizing ESP32 RMT peripheral.

### Schema

![schema](https://github.com/morcibacsi/esp32_rmt_midea_ir_tx/raw/master/schema/esp32_ir.jpg)

### Usage

Copy the files to your **documents\Arduino\libraries\esp32_arduino_rmt_midea_ir_tx** folder

### Example

```C
#include <Arduino.h>
#include <midea_ir.h>

const uint8_t MIDEA_RMT_CHANNEL = 0;
const uint8_t MIDEA_TX_PIN = 4;

MideaIR ir;

void setup()
{
    // init library
    midea_ir_init(&ir, MIDEA_RMT_CHANNEL, MIDEA_TX_PIN);

    // set mode, temperature and fan level and internal state to enabled
    ir.enabled = true;
    ir.mode = MODE_COOL;
    ir.fan_level = 3;
    ir.temperature = 23;

    // send the IR signal with the previously set properties which will switch the A/C on
    midea_ir_send(&ir);

    delay(5000);

    // starts moving the deflector
    midea_ir_move_deflector(&ir);

    delay(5000);

    // set state to disabled
    ir.enabled = false;

    // send the IR signal which will turn the A/C off
    midea_ir_send(&ir);
}

void loop()
{
    delay(1000);
}
```

### Thanks
A big thanks goes to [@sheinz](https://github.com/sheinz) His library was used as a base as it was very easy to replace the signal sending part of it with my implementation.

#### Without the following pages this software couldn't exist. They contain very useful information on how the protocol works.


* https://github.com/sheinz/esp-midea-ir
* http://veillard.com/embedded/midea.html
* https://thebloughs.net/building-the-timer/
* https://thebloughs.net/design-a-custom-ac-timer/
* https://mpetroff.net/2015/07/decoding-a-midea-air-conditioner-remote/
