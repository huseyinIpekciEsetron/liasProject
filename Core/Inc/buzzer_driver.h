/*
 * buzzer.h
 *
 *  Created on: Jul 2, 2026
 *      Author: HUSEYIN
 */

#ifndef INC_BUZZER_H_
#define INC_BUZZER_H_

#include <stdint.h>
#include <stdbool.h>
#include "stm32h7xx_hal.h" // DAC ve GPIO için

typedef enum {
    ALARM_NONE = 0,   // Sessiz
    ALARM_LOW,        // Yavaş Bip (Örn: Saniyede 1)
    ALARM_MEDIUM,     // Hızlı Bip (Örn: Saniyede 3)
    ALARM_HIGH        // Sürekli Kesintisiz Bip (Kilitlenme/Vurulma)
} AlarmLevel_t;

// Sürücü Fonksiyonları
void Buzzer_Init(DAC_HandleTypeDef *hdac, uint32_t channel);
void Buzzer_ProcessHandler(void); // main while(1) içinde sürekli çağırılacak

#ifdef __cplusplus
extern "C" {
#endif
void Buzzer_SetVolume(uint8_t volume_level); // 0-10 arası TouchGFX'ten gelen değer
void Buzzer_Beep(bool state); // Anlık susturma/açma (MUTE) kontrolü
void Buzzer_SetAlarmLevel(AlarmLevel_t level);

#ifdef __cplusplus
}
#endif

#endif /* INC_BUZZER_H_ */
