/*
 * hc165_driver.h
 *
 *  Created on: Jun 29, 2026
 *      Author: HUSEYIN
 */

#ifndef HC165_DRIVER_H
#define HC165_DRIVER_H

#include <stdint.h>
#include "main.h" // GPIO tanımları için gerekli


typedef enum {
	BTN_BRIGHTNESS_MENU = (0x40U), // 0. Bit (0x01)
	BTN_VOLUME_MENU     = (0x40000U), // 1. Bit (0x02)
    BTN_UP	            = (0x80U), // 2. Bit (0x04)
    BTN_DOWN           	= (0x80000U), // 3. Bit (0x08)
	BTN_SMOKE           = (0x10000U), // 4. Bit (0x10)
    BTN_HOME           	= (0x20U), // 5. Bit (0x20)
	BTN_MENU         	= (0x20000U), // 6. Bit (0x40)
	BTN_BLACKOUT	    = (0x10U),  // 7. Bit (0x80)
	BTN_FRNT_LEFT		= (0x4U),
	BTN_FRNT_RIGHT		= (0x2U),
	BTN_REAR_LEFT		= (0x8U),
	BTN_REAR_RIGHT		= (0x1U),
	BTN_FRNT_REAR_ALL	= (0x400U),
	KOM1_UP				= (0x100000U),
	KOM1_DOWN			= (0x200000U),
	KOM2_UP				= (0x200U),
	KOM2_DOWN			= (0x100U),
	BTN_IDLE			= 0

} HC165_Buttons;

// Sürücü fonksiyonları
void HC165_Init(void);
void HC165_Read_State(void);

#ifdef __cplusplus
extern "C" {
#endif
uint32_t HC165_Get_Button_State(void); // C++ (TouchGFX) bu fonksiyonu çağıracak

#ifdef __cplusplus
}
#endif

#endif /* HC165_DRIVER_H */
