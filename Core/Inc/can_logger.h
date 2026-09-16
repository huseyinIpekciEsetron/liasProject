/*
 * can_logger.h
 *
 *  Created on: Jul 27, 2026
 *      Author: HUSEYIN
 */

#ifndef INC_CAN_LOGGER_H_
#define INC_CAN_LOGGER_H_

#include <stdint.h>
#include <stdbool.h>
#include "stm32h7xx_hal.h"

// Log Tipleri (CAN ID veya Verinin ilk byte'ı olarak kullanılabilir)
typedef enum {
    LOG_SYS_WARNING   = 0x01, // Arayüzde çıkan uyarılar (Voltaj, İletişim, Arıza)
    LOG_FIRE_EVENT    = 0x02, // Atış başarılı/başarısız durumu
    LOG_TUBE_STATUS   = 0x03, // Tüp arıza ve doluluk değişimleri
    LOG_SYS_STATE     = 0x04  // Sistem şalter durumları (Armed, Smoke vs.)

} CanLogType_t;

// Sürücü Fonksiyonları
void CAN_Logger_Init(FDCAN_HandleTypeDef *hfdcan);

// main.c içindeki Task Scheduler'da çağırılacak Motor
void CAN_Logger_Process(void);


// C++ Tarafının (TouchGFX) verileri güvenle çekeceği fonksiyon (GETTER)
#ifdef __cplusplus
extern "C" {
#endif

// Model'den çağırılacak Log Ekleme Fonksiyonu
void CAN_Log_Add(CanLogType_t type, uint8_t byte1, uint8_t byte2, uint8_t byte3,
                 uint8_t byte4, uint8_t byte5, uint8_t byte6, uint8_t byte7);
#ifdef __cplusplus
}
#endif

#endif /* INC_CAN_LOGGER_H_ */
