/*
 * LP5036R_LED_DRIVER.h
 *
 *  Created on: Jul 1, 2026
 *      Author: HUSEYIN
 */

#ifndef INC_LP5036_LED_DRIVER_H_
#define INC_LP5036_LED_DRIVER_H_

#include <stm32h7xx_hal.h>

//#define LP5036_ADDR_WRITE(addr)  	((uint8_t)((addr) << 1U))
//#define LP5036_ADDR_READ(addr)   	((uint8_t)(((addr) << 1U) | 0x01U))
#define LP5036_ADDR_8BIT(addr7) 	((uint8_t)((addr7) << 1U))

#define LP5036_DEV1_ADDR_7BIT     	(0x30U)
#define LP5036_DEV2_ADDR_7BIT     	(0x31U)

#define LP5036_DEV1_ADDR_8BIT 		LP5036_ADDR_8BIT(LP5036_DEV1_ADDR_7BIT)
#define LP5036_DEV2_ADDR_8BIT 		LP5036_ADDR_8BIT(LP5036_DEV2_ADDR_7BIT)


#define LP5036_REG_DEVICE_CONFIG0   0x00U
#define LP5036_REG_DEVICE_CONFIG1   0x01U
#define LP5036_REG_LED_CONFIG0      0x02U
#define LP5036_REG_LED_CONFIG1      0x03U
#define LP5036_REG_BANK_BRIGHTNESS  0x04U
#define LP5036_REG_BANK_A_COLOR     0x05U
#define LP5036_REG_BANK_B_COLOR     0x06U
#define LP5036_REG_BANK_C_COLOR     0x07U
#define LP5036_REG_RESET            0x38U

#define LP5036_REG_LED0_BRIGHTNESS  0x08U   /* LEDx_BRIGHTNESS: 0x08 + x, x=0..11 */
#define LP5036_REG_OUT0_COLOR       0x14U   /* OUTx_COLOR: 0x14 + x, x=0..35 */


/* DEVICE_CONFIG1 (0x01) bit tanımları */
#define LP5036_LOG_SCALE_EN        	(1U << 5U)   /* 0=Linear, 1=Logarithmic */
#define LP5036_POWER_SAVE_EN       	(1U << 4U)   /* 0=Disabled, 1=Enabled */
#define LP5036_AUTO_INCR_EN        	(1U << 3U)   /* 0=Disabled, 1=Enabled */
#define LP5036_PWM_DITHERING_EN    	(1U << 2U)   /* 0=Disabled, 1=Enabled */
#define LP5036_MAX_CURRENT_OPTION  	(1U << 1U)   /* 0=25.5mA, 1=35mA */
#define LP5036_LED_GLOBAL_OFF      	(1U << 0U)   /* 0=Normal, 1=Shutdown */

/* DEVICE_CONFIG0 (0x00) bit tanımları */
#define LP5036_CHIP_EN             	(1U << 6U)   /* 0=Disabled, 1=Enabled */

/* LED_CONFIG0 (0x02) bit tanımları */
#define LP5036_LED7_BANK_EN		(1U << 7U)
#define LP5036_LED6_BANK_EN		(1U << 6U)
#define LP5036_LED5_BANK_EN		(1U << 5U)
#define LP5036_LED4_BANK_EN		(1U << 4U)
#define LP5036_LED3_BANK_EN		(1U << 3U)
#define LP5036_LED2_BANK_EN		(1U << 2U)
#define LP5036_LED1_BANK_EN		(1U << 1U)
#define LP5036_LED0_BANK_EN		(1U << 0U)

/* LED_CONFIG1 (0x03) bit tanımları */
#define LP5036_LED11_BANK_EN	(1U << 3U)
#define LP5036_LED10_BANK_EN	(1U << 2U)
#define LP5036_LED9_BANK_EN		(1U << 1U)
#define LP5036_LED8_BANK_EN		(1U << 0U)


/* ---- Kanal / Cihaz Sabitleri ---- */
#define LP5036_CHANNELS_PER_DEVICE   36U   /* Her entegrede 36 renk kanalı (OUTx) var */
#define LP5036_DEVICE_COUNT           2U   /* Sistemde toplam 2 entegre var */
#define LP5036_TOTAL_CHANNELS   (LP5036_CHANNELS_PER_DEVICE * LP5036_DEVICE_COUNT)   /* 72 */

#define LP5036_PHYSICAL_LED_COUNT   36U   /* Sistemdeki toplam fiziksel LED sayisi (72 kanal / 2) */
#define LP5036_DIM_LEVEL            0x10U // 0x64
#define LP5036_GREEN_LEVEL          0x20U // 0x96
#define LP5036_RED_LEVEL            0x20U // 0x96

/* ---- Hata Kodları ---- */
typedef enum
{
    LP5036_OK = 0,
    LP5036_ERROR_INVALID_CHANNEL,
    LP5036_ERROR_I2C_FAIL
} LP5036_Status_t;

/* ---- Global kanal -> fiziksel cihaz eşlemesi ---- */
typedef struct
{
    uint8_t dev_addr;       /* Cihazın 8-bit I2C adresi (HAL formatında) */
    uint8_t local_index;    /* O cihaz içindeki gerçek indeks (0-35 kanal / 0-11 LED modülü) */
} LP5036_ChannelMap_t;

typedef enum
{
    LP5036_LED_OFF = 0,   /* Hem R hem G kapali */
    LP5036_LED_RED,       /* Sadece Red yaniyor */
    LP5036_LED_GREEN,     /* Sadece Green yaniyor */
	LP5036_LED_ORANGE,
    LP5036_LED_BOTH,      /* Hem R hem G tam parlaklikta acik */
    LP5036_LED_DIM        /* Ikisi de dusuk seviyede (karartma modu) */

} LP5036_LedState_t;

/* ---- Fiziksel LED -> R/G kanal eslesme tablosu girisi ---- */
typedef struct
{
    uint8_t r_channel;   /* Bu LED'in R'sinin bagli oldugu global kanal (0-71) */
    uint8_t g_channel;   /* Bu LED'in G'sinin bagli oldugu global kanal (0-71) */
} LP5036_LedPinMap_t;

/* ---- Fonksiyon Prototipleri ---- */
void LP5036LedDriver_Init(I2C_HandleTypeDef *HI2Cx);

LP5036_Status_t LP5036_WriteReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t value);
LP5036_Status_t LP5036_WriteRegs(uint8_t dev_addr, uint8_t reg_addr, const uint8_t *data, uint16_t len);
LP5036_Status_t LP5036_ReadReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *value);
LP5036_Status_t LP5036_SetButtonLeds(LP5036_LedState_t ledState);

#ifdef __cplusplus
extern "C" {
#endif
/* ---- Konfigürasyon Fonksiyonları ---- */
LP5036_Status_t LP5036_DeviceConfig(uint8_t dev_addr);
LP5036_Status_t LP5036_Config(void);
LP5036_Status_t LP5036_SetColor(uint8_t global_channel, uint8_t color_value);
LP5036_Status_t LP5036_SetLedState(uint8_t led_index, LP5036_LedState_t state);
LP5036_Status_t LP5036_AllLedsOff(void);
LP5036_Status_t LP5036_AllLedsOn(void);
LP5036_Status_t LP5036_AllLedsDim(void);
LP5036_Status_t LP5036_SetGlobalBrightness(uint8_t level);

#ifdef __cplusplus
}
#endif

#endif /* INC_LP5036_LED_DRIVER_H_ */
