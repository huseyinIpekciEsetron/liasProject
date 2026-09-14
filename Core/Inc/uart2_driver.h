/*
 * uart2_driver.h
 *
 *  Created on: Jul 13, 2026
 *      Author: HUSEYIN
 */

#ifndef INC_UART2_DRIVER_H_
#define INC_UART2_DRIVER_H_

#include <stdint.h>
#include <stdbool.h>
#include "stm32h7xx_hal.h"

// Buffer Boyutları (İhtiyacına göre artırabilirsin)
#define UART2_RX_BUFFER_SIZE 256
#define UART2_TX_BUFFER_SIZE 256

// =================================================================
// BMB STATUS RX PAKET YAPISI (KARŞIDAN GELEN - 64 BYTE)
// =================================================================
#pragma pack(push, 1)
typedef struct {
    uint8_t  header[4];       // Byte 0-3  : 'E', 'S', 'E', 0x01
    uint8_t  timestamp;       // Byte 4    : 0-200 arası sayıcı
    uint8_t  sis_status[16];  // Byte 5-20 : SIS1-16 (0x55 Not connected, 0xAA Connected)
    uint8_t  frag_status[16]; // Byte 21-36: FRAG1-16 (0x55 Not connected, 0xAA Connected)
    uint8_t  bmb_ready;     // Byte 37   : 0x00 (Not Ready), 0xAA (Ready)
    uint8_t  blasting_state;  // Byte 38   : 0x00 (Not Blasting), 0xAA (Blasting) PATLATMA DURUMU
    uint8_t  console_volt_err;// Byte 39   : 0(No), 1(Low), 2(High)
    uint8_t  blast_ready_err;  // Byte 40   : 0(No), 1(Low), 2(High)
    uint8_t  main_5v_err;     // Byte 41   : 0(No), 1(Low), 2(High)
    uint8_t  mcu_3v3_err;     // Byte 42   : 0(No), 1(Low), 2(High)
    uint8_t  status_read_err; // Byte 43   : 0(No), 1(Low), 2(High)
    uint8_t  overlap_err;     // Byte 44   : Bit0(BMB1) ... Bit15(BMB16)
    uint8_t  comm_err;        // Byte 45   : Bit0(NoMsg), Bit1(CRC) ...
    uint32_t console_input_voltage; // Byte 46-49
	uint16_t blast_volt_reg1;       // Byte 50-51
	uint16_t blast_volt_reg2;       // Byte 52-53
	uint16_t main_5v_reg;           // Byte 54-55
	uint16_t power_3v3_reg;         // Byte 56-57
    uint8_t  footer[2];       // Byte 58-59: 'O', 'N'
    uint32_t crc32;           // Byte 60-63: İlk 60 byte'ın CRC32 kodu
} BMB_RxPacket_t;
#pragma pack(pop)

// =================================================================
// BMB COMMAND TX PAKET YAPISI (KARŞIYA GÖNDERİLEN - 64 BYTE)
// =================================================================
#pragma pack(push, 1)
typedef struct {
    uint8_t  header[4];       // Byte 0-3  : 'E', 'S', 'E', 0x02
    uint8_t  timestamp;       // Byte 4    : 0-200 arası sayıcı
    uint8_t  sis_status[16];  // Byte 5-20 : SIS1-16 (0x55 Don't blast, 0xAA Blast)
    uint8_t  frag_status[16]; // Byte 21-36: FRAG1-16 (0x55 Don't blast, 0xAA Blast)
    uint8_t  power_ready;     // Byte 37   : 0x00 (Not Ready), 0xAA (Ready)
    uint8_t  reserved[20];    // Byte 38-57: Boş/Rezerve alan
    uint8_t  footer[2];       // Byte 58-59: 'O', 'N'
    uint32_t crc32;           // Byte 60-63: İlk 60 byte'ın CRC32 kodu
} BMB_TxPacket_t;
#pragma pack(pop)

// Fonksiyon Prototipleri
void UART2_Init(UART_HandleTypeDef *huartx);
void UART2_Send_Data(uint8_t *data, uint16_t length);

// Kesme (Callback) Yönlendiricileri
void UART2_TxCpltCallback(UART_HandleTypeDef *huart);
void UART2_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size);
void UART2_ErrorCallback(UART_HandleTypeDef *huart);

void UART2_Send_BMB_Test_Packet(void);

// C++ Tarafının (TouchGFX) verileri güvenle çekeceği fonksiyon (GETTER)
#ifdef __cplusplus
extern "C" {
#endif
void BMB_Get_Latest_Rx_Data(BMB_RxPacket_t *out_data);
bool BMB_Check_New_Data(void);
void BMB_Set_Tx_Command(bool power_ready, uint16_t sis_mask, uint16_t frag_mask);

#ifdef __cplusplus
}
#endif

#endif /* INC_UART2_DRIVER_H_ */
