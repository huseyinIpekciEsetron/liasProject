/*
 * rtc.c
 *
 *  Created on: Sep 4, 2026
 *      Author: HUSEYIN
 */

#include "rtc.h"

RTC_HandleTypeDef *Hrtc;

void RTC_Init(RTC_HandleTypeDef *hRtc)
{
	Hrtc = hRtc;

	RTC_TimeTypeDef sTime = {0};
	RTC_DateTypeDef sDate = {0};

	// Zaman Ayarı
	sTime.Hours = 0;
	sTime.Minutes = 0;
	sTime.Seconds = 0;
	sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
	sTime.StoreOperation = RTC_STOREOPERATION_RESET;
	HAL_RTC_SetTime(Hrtc, &sTime, RTC_FORMAT_BIN);

	// Tarih Ayarı (Donanım RTC'si Yılı 0-99 arası tutar, bu yüzden mod 100 yapıyoruz)
	sDate.WeekDay = RTC_WEEKDAY_MONDAY;
	sDate.Month = 1;
	sDate.Date = 1;
	sDate.Year = (uint8_t)(2026 % 100);
	HAL_RTC_SetDate(Hrtc, &sDate, RTC_FORMAT_BIN);
}

void Sync_RTC_Time(uint8_t hour, uint8_t min, uint8_t sec, uint8_t day, uint8_t month, uint16_t year)
{
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    // Zaman Ayarı
    sTime.Hours = hour;
    sTime.Minutes = min;
    sTime.Seconds = sec;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;
    HAL_RTC_SetTime(Hrtc, &sTime, RTC_FORMAT_BIN);

    // Tarih Ayarı (Donanım RTC'si Yılı 0-99 arası tutar, bu yüzden mod 100 yapıyoruz)
    sDate.WeekDay = RTC_WEEKDAY_MONDAY;
    sDate.Month = month;
    sDate.Date = day;
    sDate.Year = (uint8_t)(year % 100);
    HAL_RTC_SetDate(Hrtc, &sDate, RTC_FORMAT_BIN);
}

void Read_RTC_Time(uint8_t* hour, uint8_t* min, uint8_t* sec, uint8_t* day, uint8_t* month, uint16_t* year)
{
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    // NOT: STM32'de Shadow register'ların kilitlenmemesi için Time ve Date peş peşe okunmalıdır!
    HAL_RTC_GetTime(Hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(Hrtc, &sDate, RTC_FORMAT_BIN);

    *hour = sTime.Hours;
    *min = sTime.Minutes;
    *sec = sTime.Seconds;
    *day = sDate.Date;
    *month = sDate.Month;
    *year = 2000 + sDate.Year; // Örn: 26 okunduğunda 2026 olarak döndürür
}
