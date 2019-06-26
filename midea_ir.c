#include "midea_ir.h"
#include "esp32_rmt_midea_ir_tx.h"

/**
 * Midea Air conditioner protocol consists of 3 data bytes.
 * After each data byte follows its inverted byte (0 an 1 are switched). It
 * provides errors checking on the receiver side.
 *
 * Each 6 total bytes follows with additional repeat of the same 6 bytes to
 * provide even more errors checking. (except move deflector command)
 */

/**
 * Bits encoding.
 *
 * T is 21 38000kHz pulses.
 *
 * Bit 0 is encoded as 1T of high level and 1T of low level.
 * Bit 1 is encoded as 1T of high level and 3T of low level.
 *
 * Start condition: 8T of hight level and 8T of low level.
 * Stop bit: 6 bytes follow with one "0" stop bit.
 */

/**
 * Data packet (3 bytes):
 * [1010 0010] [ffff ssss] [ttttcccc]
 *
 * 1010 0010 - (0xB2) is a constant
 *
 * ffff - Fan control
 *      1011 - automatic or 0
 *      1001 - low speed
 *      0101 - medium speed
 *      0011 - high speed
 *      0001 - off (or when fan speed is irrelevant)
 *
 * ssss - State control
 *      1111 - on
 *      1011 - off
 *
 * tttt - Temperature control (see table below)
 *      0000 - 17 Celsius
 *      ...
 *      1011 - 30 Celsius
 *      1110 - off
 *
 * cccc - Command
 *      0000 - cool
 *      1100 - heat
 *      1000 - automatic
 *      0100 - fan
 */

#define TEMP_LOW  17
#define TEMP_HIGH 30

typedef struct
{
    uint8_t magic;      // 0xB2 always
    uint8_t state   : 4;
    uint8_t fan     : 4;
    uint8_t command : 4;
    uint8_t temp    : 4;
} DataPacketStruct;

typedef union {
    DataPacketStruct data;
    uint8_t DataAsByteArray[sizeof(DataPacketStruct)];
} DataPacket;

// Table to convert temperature in Celsius to a strange Midea AirCon values
const static uint8_t temperature_table[] = {
    0b0000,   // 17 C
    0b0001,   // 18 C
    0b0011,   // 19 C
    0b0010,   // 20 C
    0b0110,   // 21 C
    0b0111,   // 22 C
    0b0101,   // 23 C
    0b0100,   // 24 C
    0b1100,   // 25 C
    0b1101,   // 26 C
    0b1001,   // 27 C
    0b1000,   // 28 C
    0b1010,   // 29 C
    0b1011    // 30 C
//  0b1110    // off
};

// Table to convert fan level
const static uint8_t fan_table[] = {
    0b1011,   // 0 - AUTO
    0b1001,   // 1
    0b0101,   // 2
    0b0011,   // 3
};

void midea_ir_init(MideaIR *ir, const uint8_t channel, const uint8_t txPin)
{
    ir->temperature = 24;
    ir->enabled = false;
    ir->mode = MODE_AUTO;
    ir->fan_level = 0;

    rmt_midea_ir_tx_channel_init(channel, txPin);
}

void midea_ir_send(MideaIR *ir)
{
    DataPacket dp;
    pack_data(ir, &dp.data);

    rmt_midea_ir_tx_send_raw_message(dp.DataAsByteArray, 3);
}

void midea_ir_move_deflector(MideaIR *ir)
{
    uint8_t data[3] = {0xB2, 0x0F, 0xE0};

    rmt_midea_ir_tx_send_raw_message(data, 3);
}

void midea_ir_stop(uint8_t channel)
{
    rmt_midea_ir_tx_channel_stop(channel);
}

void pack_data(MideaIR *ir, DataPacketStruct *data)
{
    data->magic = 0xB2;
    if (ir->enabled) {
        if (ir->mode == MODE_AUTO) {
            data->fan = 0b0001;         // for auto mode fan must be 0b0001
        } else {
            data->fan = fan_table[ir->fan_level];
        }
        data->state = 0b1111;  // on
        data->command = ir->mode;

        if (ir->mode == MODE_VENTILATE) {
            data->temp = 0b1110;
        } else {
            if (ir->temperature >= TEMP_LOW && ir->temperature <= TEMP_HIGH) {
                data->temp = temperature_table[ir->temperature - TEMP_LOW];
            } else {
                data->temp = 0b0100;
            }
        }
    } else {
        data->fan = 0b0111;
        data->state = 0b1011; // off
        data->command = 0b0000;
        data->temp = 0b1110;
    }
}