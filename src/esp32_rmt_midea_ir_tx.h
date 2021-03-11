/*
    ESP32 library to send IR pulses to Midea air conditioner
*/

#pragma once

#ifndef _esp32_rmt_midea_ir_tx_h
    #define _esp32_rmt_midea_ir_tx_h

    #include "driver/rmt.h"
    #include "driver/periph_ctrl.h"
    #include "soc/rmt_reg.h"

    #ifdef __cplusplus
    extern "C" {
    #endif

    void rmt_midea_ir_tx_send_raw_message(uint8_t data[], uint8_t lengthOfData);
    void rmt_midea_ir_tx_channel_init(uint8_t channel, uint8_t txPin);
    void rmt_midea_ir_tx_channel_stop(uint8_t channel);

    #ifdef __cplusplus
    } // extern C
    #endif
#endif