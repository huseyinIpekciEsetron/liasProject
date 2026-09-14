/*
 * hc165_driver.c
 *
 *  Created on: Jun 29, 2026
 *      Author: HUSEYIN
 */


#include "hc165_driver.h"

static volatile uint32_t current_button_state = 0;
static const int buttonNum = 24; // Kaç buton

void HC165_Init(void)
{

    current_button_state = 0;
}

void HC165_Read_State(void)
{
    uint32_t tempState = 0;

    HC165_CLK_GPIO_Port->ODR |= (HC165_CLK_Pin);
    HC165_EN_GPIO_Port->ODR &= ~(HC165_EN_Pin);
    HC165_EN_GPIO_Port->ODR |= (HC165_EN_Pin);

    for (int i = 0; i < buttonNum; i++)
    {
        HC165_CLK_GPIO_Port->ODR &= ~(HC165_CLK_Pin);

        if (HC165_MISO_GPIO_Port->IDR & (HC165_MISO_Pin))
            tempState &= ~(1 << i);
        else
            tempState |= (1 << i);

        HC165_CLK_GPIO_Port->ODR |= (HC165_CLK_Pin);
    }

    current_button_state = tempState;
}

// C++ tarafı (TouchGFX) veriyi bu güvenli "Getter" fonksiyonu ile çekecek
uint32_t HC165_Get_Button_State(void)
{
    return current_button_state;
}
