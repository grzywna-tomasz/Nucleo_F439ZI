#include "app.h"
#include "stm32f4xx_hal.h"
#include <string.h>

extern ETH_HandleTypeDef heth;

volatile uint8_t dummy_wait = 1;

void App_main(void)
{
    HAL_ETH_Start(&heth);

    while(1)
    {
        uint8_t frame[60] = {
            // Destination MAC (broadcast)
            0xff,0xff,0xff,0xff,0xff,0xff,
            // Source MAC
            0x00,0x80,0xE1,0x00,0x00,0x00,
            // Ethertype (dummy)
            0x08,0x00,
            // Payload
            'H','E','L','L','O'
        };

        // --- TX config ---
        ETH_TxPacketConfigTypeDef TxConfig;
        ETH_BufferTypeDef TxBufferStruct;

        memset(&TxConfig, 0, sizeof(TxConfig));
        memset(&TxBufferStruct, 0, sizeof(TxBufferStruct));

        // Link buffer
        TxBufferStruct.buffer = frame;
        TxBufferStruct.len    = 60;
        TxBufferStruct.next   = NULL;

        // Configure packet
        TxConfig.Length   = 60;
        TxConfig.TxBuffer = &TxBufferStruct;

        // Optional but recommended
        TxConfig.Attributes = ETH_TX_PACKETS_FEATURES_CSUM | ETH_TX_PACKETS_FEATURES_CRCPAD;
        TxConfig.ChecksumCtrl = ETH_CHECKSUM_IPHDR_PAYLOAD_INSERT_PHDR_CALC;

        HAL_ETH_Transmit(&heth, &TxConfig, HAL_MAX_DELAY);

        while(dummy_wait);
        dummy_wait = 1;
    }
}