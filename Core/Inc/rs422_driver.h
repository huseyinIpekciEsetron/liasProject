/*
 * rs422.h
 *
 *  Created on: Jul 1, 2026
 *      Author: HUSEYIN
 */

#ifndef INC_RS422_DRIVER_H_
#define INC_RS422_DRIVER_H_

#include <stdint.h>
#include <stdbool.h>
#include "stm32h7xx_hal.h"

// RS422 Buffer Boyutları (İhtiyacına göre ayarlayabilirsin)
#define RS422_RX_BUFFER_SIZE 256
#define RS422_TX_BUFFER_SIZE 256

// Fonksiyon Prototipleri
void RS422_Init(UART_HandleTypeDef *huartx);
void RS422_Send_Data(uint8_t *data, uint16_t length);
void RS422_TxCpltCallback(UART_HandleTypeDef *huart);
void RS422_Data_Received_Callback(UART_HandleTypeDef *huart, uint16_t size);
void RS422_ErrorCallback(UART_HandleTypeDef *huart);

// C++ Tarafının (TouchGFX) verileri güvenle çekeceği fonksiyon (GETTER)
#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif /* INC_RS422_DRIVER_H_ */
