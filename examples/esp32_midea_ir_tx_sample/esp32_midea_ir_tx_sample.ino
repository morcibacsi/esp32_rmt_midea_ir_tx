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
    midea_ir_move_deflector();

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