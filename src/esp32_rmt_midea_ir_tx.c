#include "esp32_rmt_midea_ir_tx.h"

#define PULSE_LENGTH 557; // Each pulse (one "time" unit or 1T) is 21 cycles of a 38KHz carrier burst (about 550µs long)

volatile uint8_t _rmt_midea_ir_tx_txPin;
volatile uint8_t _rmt_midea_ir_tx_channel;

/*
    Convert a byte to its midea-protocol encoded counterpart as a RMT duration
    Idea borrowed from here: https://github.com/kimeckert/ESP32-RMT-Rx-raw
*/
void ConvertByteToMideaRmtValues(uint8_t byteValue, int16_t rmtValues[], int16_t rmtValuesIndex, int16_t *rmtCount)
{
    // A logic "1" is a 1T pulse followed by a 3T space and takes 2.25ms to transmit
    // A logic "0" is a 1T pulse followed by a 1T space and takes 1.125ms

    int16_t currentIndex = rmtValuesIndex;

    for (int8_t j = 7; j >= 0; j--)
    {
        currentIndex++;
        rmtValues[currentIndex] = 1*PULSE_LENGTH; // 1T pulse
        currentIndex++;

        int8_t currentBitValue = (byteValue >> j) & 1;
        if (currentBitValue == 1)
        {
            rmtValues[currentIndex] = -3*PULSE_LENGTH; // 3T space
        }
        else
        {
            rmtValues[currentIndex] = -1*PULSE_LENGTH; // 1T space
        }
    }
    *rmtCount = currentIndex;
}

/* Converts RMT durations to an array of rmt_item32_t */
void ConvertRmtValuesToRmtObjects(int16_t rmtValues[], int16_t rmtValuesCount, rmt_item32_t rmtObjects[], int16_t *rmtObjectsCount)
{
    int16_t rmtObjectsIndex = 0;
    bool isFirstPartOfRmtObjectOccupied = false;

    for (int16_t i = 0; i < rmtValuesCount; i++)
    {
        if (isFirstPartOfRmtObjectOccupied == false)
        {
            rmtObjects[rmtObjectsIndex].duration0 = abs(rmtValues[i]);
            rmtObjects[rmtObjectsIndex].level0 = (rmtValues[i] > 0) ? 0 : 1;
            isFirstPartOfRmtObjectOccupied = true;

            rmtObjects[rmtObjectsIndex].duration1 = 0;
            rmtObjects[rmtObjectsIndex].level1 = 0;
        }
        else
        {
            rmtObjects[rmtObjectsIndex].duration1 = abs(rmtValues[i]);
            rmtObjects[rmtObjectsIndex].level1 = (rmtValues[i] > 0) ? 0 : 1;
            rmtObjectsIndex++;
            isFirstPartOfRmtObjectOccupied = false;
        }
    }
    *rmtObjectsCount = rmtObjectsIndex + 1;
}

/* Add the bytes and their complements to the RmtArray */
void AddBytesToRmtArray(uint8_t data[], uint8_t lengthOfData, int16_t rmtValues[], int16_t rmtValuesIndex, int16_t *rmtCount)
{
    int16_t rmtValueIndex = rmtValuesIndex;

    for (int8_t i = 0; i < lengthOfData; i++)
    {
        ConvertByteToMideaRmtValues(data[i], rmtValues, rmtValueIndex, &rmtValueIndex);
        int8_t flipped = (int8_t) ~data[i];
        ConvertByteToMideaRmtValues(flipped, rmtValues, rmtValueIndex, &rmtValueIndex);
    }
    *rmtCount = rmtValueIndex;
}

/* Send the message using RMT */
void rmt_midea_ir_tx_send_raw_message(uint8_t data[], uint8_t lengthOfData)
{
    int16_t rmtValues[200] = { 0 };
    int16_t rmtValueIndex = -1;

    // #pragma region Add the whole message to the RmtArray for the first time

    // The AGC burst consists of a 4.5ms burst followed by 4.5ms space (8T pulse + 8T space)
    // frame start markers 4T down, 4T up
    rmtValues[0] = 8*PULSE_LENGTH;
    rmtValues[1] = -8*PULSE_LENGTH;

    rmtValueIndex = 1;

    // add the bytes and its complements to the RmtArray
    AddBytesToRmtArray(data, lengthOfData, rmtValues, rmtValueIndex, &rmtValueIndex);

    // add the end of frame marker to the RmtArray
    rmtValueIndex++;
    rmtValues[rmtValueIndex] = PULSE_LENGTH;
    rmtValueIndex++;
    rmtValues[rmtValueIndex] = -9*PULSE_LENGTH;

    // #pragma endregion

    // #pragma region Add the whole message to the RmtArray again

    // frame start markers 4T down, 4T up
    rmtValueIndex++;
    rmtValues[rmtValueIndex] = 8*PULSE_LENGTH;
    rmtValueIndex++;
    rmtValues[rmtValueIndex] = -8*PULSE_LENGTH;

    // add the bytes and its complements to the RmtArray
    AddBytesToRmtArray(data, lengthOfData, rmtValues, rmtValueIndex, &rmtValueIndex);

    // add the end of frame marker to the RmtArray
    rmtValueIndex++;
    rmtValues[rmtValueIndex] = PULSE_LENGTH;
    rmtValueIndex++;
    rmtValues[rmtValueIndex] = 0;

    // #pragma endregion

    rmt_item32_t rmtObjects[100];
    int16_t rmtObjectsCount;
    ConvertRmtValuesToRmtObjects(rmtValues, rmtValueIndex + 1, rmtObjects, &rmtObjectsCount);

    rmt_write_items(_rmt_midea_ir_tx_channel, rmtObjects, rmtObjectsCount, true);
}

/* Initialize RMT transmit channel */
void rmt_midea_ir_tx_channel_init(uint8_t channel, uint8_t txPin)
{
    _rmt_midea_ir_tx_channel = channel;
    _rmt_midea_ir_tx_txPin = txPin;

    rmt_config_t rmt_tx;

    rmt_tx.channel       = channel;
    rmt_tx.gpio_num      = txPin;
    rmt_tx.clk_div       = 80; // 1 MHz, 1 us - we set up sampling to every 1 microseconds
    rmt_tx.mem_block_num = 2;
    rmt_tx.rmt_mode      = RMT_MODE_TX;
    rmt_tx.tx_config.loop_en = false;
    rmt_tx.tx_config.carrier_duty_percent = 25;
    rmt_tx.tx_config.carrier_freq_hz = 38000;
    rmt_tx.tx_config.carrier_level   = RMT_CARRIER_LEVEL_LOW;
    rmt_tx.tx_config.carrier_en      = true;
    rmt_tx.tx_config.idle_level      = RMT_IDLE_LEVEL_HIGH;
    rmt_tx.tx_config.idle_output_en  = true;

    rmt_config(&rmt_tx);
    rmt_driver_install(rmt_tx.channel, 0, 0);

    // start the channel
    rmt_tx_start(_rmt_midea_ir_tx_channel, 1);
}

/* Uninstall the RMT driver */
void rmt_midea_ir_tx_channel_stop(uint8_t channel)
{
    rmt_driver_uninstall(channel);
}