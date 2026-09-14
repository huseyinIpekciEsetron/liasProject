/*
 * rtc.h
 *
 *  Created on: Sep 4, 2026
 *      Author: HUSEYIN
 */

#ifndef INC_RTC_H_
#define INC_RTC_H_

#include <stdint.h>
#include "stm32h7xx_hal.h"

void RTC_Init(RTC_HandleTypeDef *hRtc);

#ifdef __cplusplus
extern "C" {
#endif

// Donanım RTC'sini ICD'den gelen veriye göre kalibre eder
void Sync_RTC_Time(uint8_t hour, uint8_t min, uint8_t sec, uint8_t day, uint8_t month, uint16_t year);

// Donanım RTC'sinden o anki zamanı okur
void Read_RTC_Time(uint8_t* hour, uint8_t* min, uint8_t* sec, uint8_t* day, uint8_t* month, uint16_t* year);

#ifdef __cplusplus
}
#endif

#endif /* INC_RTC_H_ */
