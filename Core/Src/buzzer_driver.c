/*
 * buzzer.c
 *
 *  Created on: Jul 2, 2026
 *      Author: HUSEYIN
 */


#include "buzzer_driver.h"
#include "main.h"

static DAC_HandleTypeDef *buzzer_dac;
static uint32_t buzzer_dac_channel;

// Alarm State Değişkenleri
static AlarmLevel_t current_alarm = ALARM_NONE;
static uint32_t last_toggle_time = 0;
static bool is_beeping_now = false;
static uint8_t currentVolume = 1;

void Buzzer_Init(DAC_HandleTypeDef *hdac, uint32_t channel)
{
    buzzer_dac = hdac;
    buzzer_dac_channel = channel;

    // Boost Converter'ı aktif et (Sisteme güç ver)
    HAL_GPIO_WritePin(BUZZER_EN_GPIO_Port, BUZZER_EN_Pin, GPIO_PIN_SET);

    // DAC'yi başlat
    HAL_DAC_Start(buzzer_dac, buzzer_dac_channel);

    // Başlangıç sesi %50 (Seviye 5) olsun
    Buzzer_SetVolume(1);

    // Başlangıçta sessiz ol
    HAL_GPIO_WritePin(BUZZER_MUTE_GPIO_Port, BUZZER_MUTE_Pin, GPIO_PIN_RESET);
}

void Buzzer_SetVolume(uint8_t volume_level)
{
	 uint32_t dac_value = 4095;

    // Koruma: Seviye 0-10 arasında olmalı
    if (volume_level > 5) volume_level = 5;

    switch(volume_level)
    {
    	case 0:
    		 // Seviye 0 ise tamamen sustur (MUTE)
    		        HAL_GPIO_WritePin(BUZZER_MUTE_GPIO_Port, BUZZER_MUTE_Pin, GPIO_PIN_RESET);
    		break;
    	case 1:
    		//HAL_GPIO_WritePin(BUZZER_MUTE_GPIO_Port, BUZZER_MUTE_Pin, GPIO_PIN_SET);
			HAL_GPIO_WritePin(BUZZER_EN_GPIO_Port, BUZZER_EN_Pin, GPIO_PIN_RESET);
    		break;
    	case 2:
    		HAL_GPIO_WritePin(BUZZER_EN_GPIO_Port, BUZZER_EN_Pin, GPIO_PIN_SET);
    		dac_value = 4000;
    		break;
    	case 3:
    		HAL_GPIO_WritePin(BUZZER_EN_GPIO_Port, BUZZER_EN_Pin, GPIO_PIN_SET);
    		dac_value = 3500;
    		break;
    	case 4:
    		HAL_GPIO_WritePin(BUZZER_EN_GPIO_Port, BUZZER_EN_Pin, GPIO_PIN_SET);
    		dac_value = 2750;
    		break;
    	case 5:
    		HAL_GPIO_WritePin(BUZZER_EN_GPIO_Port, BUZZER_EN_Pin, GPIO_PIN_SET);
    		dac_value = 0;
    		break;
    	default:
    		break;
    }

    HAL_DAC_SetValue(buzzer_dac, buzzer_dac_channel, DAC_ALIGN_12B_R, dac_value);

    currentVolume = volume_level;
}

void Buzzer_Beep(bool state)
{
    // Cihazın güç devresi (TPS61040) hep açık kalır,
    // sadece Mosfet üzerinden hızlıca bekleme yapmadan sesi kesip açarız.
    if(state) {
        HAL_GPIO_WritePin(BUZZER_MUTE_GPIO_Port, BUZZER_MUTE_Pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(BUZZER_MUTE_GPIO_Port, BUZZER_MUTE_Pin, GPIO_PIN_RESET);
    }
}

// =======================================================
// UDP'den Veri Geldiğinde Çağıracağın Fonksiyon
// =======================================================
void Buzzer_SetAlarmLevel(AlarmLevel_t level)
{
    if (current_alarm != level)
    {
        current_alarm = level;

        // Seviye değiştiğinde anında tepki ver
        if (level == ALARM_NONE) {
            Buzzer_Beep(false);
            is_beeping_now = false;
        } else {
            Buzzer_Beep(true);
            is_beeping_now = true;
            last_toggle_time = HAL_GetTick(); // Süreyi sıfırla
        }
    }
}

// =======================================================
// main.c içindeki while(1) döngüsünde çalışacak Motor
// =======================================================
void Buzzer_ProcessHandler(void)
{
	if ( (current_alarm == ALARM_NONE) || (currentVolume == 0) )
	{
		HAL_GPIO_WritePin(BUZZER_MUTE_GPIO_Port, BUZZER_MUTE_Pin, GPIO_PIN_RESET);
		return; // Hiç tehdit yoksa işlem yapma
	}

	uint32_t current_time = HAL_GetTick();

	// Açık ve kapalı kalma sürelerini (asimetrik) tutacak değişkenler
	uint32_t on_time = 0;
	uint32_t off_time = 0;

	// Tehdite göre Makro Ritim / Chirp (Tıklama) Süreleri
	switch (current_alarm)
	{
		case ALARM_LOW:
			// Sakin Uyarı: 0.5 Saniye Çal, 1.5 Saniye Sus (Geniş aralıklı bip)
			on_time = 500;
			off_time = 1500;
			break;

		case ALARM_MEDIUM:
			// Dikkat Çekici: 0.25 Saniye Çal, 0.25 Saniye Sus (Hızlı ve kesik kesik bip)
			on_time = 250;
			off_time = 250;
			break;

		case ALARM_HIGH:
			// Acil Durum: Saniyede 1 Bip kısıtlamasını aşmak için "Tık/Böcek" sesi hilesi
			on_time = 60;
			off_time = 60;
			break;

		default:
			return;
	}

	// Sistemin şu an açık veya kapalı olmasına göre bekleyeceği süreyi seç
	uint32_t current_interval = is_beeping_now ? on_time : off_time;

	// Süre dolduysa Buzzer'ın durumunu tersine çevir
	if ((current_time - last_toggle_time) >= current_interval)
	{
		last_toggle_time = current_time;
		is_beeping_now = !is_beeping_now;
		Buzzer_Beep(is_beeping_now);
	}
}
