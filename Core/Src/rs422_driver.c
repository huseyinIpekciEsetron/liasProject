/*
 * rs422.c
 *
 *  Created on: Jul 1, 2026
 *      Author: HUSEYIN
 */

#include "rs422_driver.h"
#include "main.h"
#include <string.h>

// DMA için Non-Cacheable RAM bölgesinde (Senin MPU Region 1) oluşturulmuş Buffer'lar.
// Not: STM32CubeIDE'de genellikle D2 SRAM bölgesi ".ram_d2" veya ".sram2" olarak isimlendirilir.
// Eğer derleyici hata verirse bu attribute kısmını silip sadece uint8_t tanımlayabilirsin.
#if defined ( __ICCARM__ ) || defined ( __CC_ARM ) || defined ( __GNUC__ )
    __attribute__((section(".ram_d2"), aligned(32))) uint8_t rs422_rx_buffer[RS422_RX_BUFFER_SIZE];
    __attribute__((section(".ram_d2"), aligned(32))) uint8_t rs422_tx_buffer[RS422_TX_BUFFER_SIZE];
#else
    __attribute__((aligned(32))) uint8_t rs422_rx_buffer[RS422_RX_BUFFER_SIZE];
    __attribute__((aligned(32))) uint8_t rs422_tx_buffer[RS422_TX_BUFFER_SIZE];
#endif


UART_HandleTypeDef* RS422_huartx;


/**
 * @brief RS422 Başlatma Fonksiyonu
 */
void RS422_Init(UART_HandleTypeDef *huartx)
{
	RS422_huartx = huartx;

    HAL_GPIO_WritePin(RS422_RE_GPIO_Port, RS422_RE_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(RS422_DE_GPIO_Port, RS422_DE_Pin, GPIO_PIN_RESET);

    if(HAL_UARTEx_ReceiveToIdle_DMA(huartx, rs422_rx_buffer, RS422_RX_BUFFER_SIZE) != HAL_OK)
    {
    	Error_Handler();
    }
	__HAL_DMA_DISABLE_IT(RS422_huartx->hdmarx, DMA_IT_HT);

}

/**
 * @brief RS422 Üzerinden DMA ile Veri Gönderme
 */
void RS422_Send_Data(uint8_t *data, uint16_t length)
{
    if(length == 0 || length > RS422_TX_BUFFER_SIZE) return;

    // 1. Gönderilecek veriyi güvenli (DMA'nın erişebileceği) TX buffer'ına kopyala
    memcpy(rs422_tx_buffer, data, length);

    SCB_CleanDCache_by_Addr((uint32_t *)rs422_tx_buffer, length);

    // 2. RS422 Çipini YAZMA (Transmit) moduna al (DE Pin = HIGH)
    HAL_GPIO_WritePin(RS422_DE_GPIO_Port, RS422_DE_Pin, GPIO_PIN_SET);

    // 3. DMA ile gönderimi başlat
    HAL_UART_Transmit_DMA(RS422_huartx, rs422_tx_buffer, length);
}

void RS422_TxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart == RS422_huartx)
	{
		// Gönderim bitti, RS422 çipini tekrar DİNLEME (Receive) moduna al
		HAL_GPIO_WritePin(RS422_DE_GPIO_Port, RS422_DE_Pin, GPIO_PIN_RESET);
	}
}

void RS422_Data_Received_Callback(UART_HandleTypeDef *huart, uint16_t size)
{
	if (huart == RS422_huartx)
	{
		// rs422_rx_buffer[] veriler burda

		SCB_InvalidateDCache_by_Addr((uint32_t *)rs422_rx_buffer, RS422_RX_BUFFER_SIZE);

		// PAKET KONTROLU VERİLERİ İŞLEME

		// NORMAL MOD
		// Mevcut dinleme bittiği için, bir sonraki paket için DMA'yı TEKRAR BAŞLAT!
		// =======================================================
		HAL_UARTEx_ReceiveToIdle_DMA(RS422_huartx, rs422_rx_buffer, RS422_RX_BUFFER_SIZE);
		__HAL_DMA_DISABLE_IT(RS422_huartx->hdmarx, DMA_IT_HT);
	}
}

/**
 * @brief UART Hatası Meydana Geldiğinde Çalışan Kesme
 */
void RS422_ErrorCallback(UART_HandleTypeDef *huart)
{
	uint32_t error_code = huart->ErrorCode;

	if (error_code & HAL_UART_ERROR_ORE) {
		// Overrun Error (Veri taşıntısı - Debugger durduğu için veya çok hızlı veri geldiği için)
		__HAL_UART_CLEAR_OREFLAG(huart);
	}
	if (error_code & HAL_UART_ERROR_FE) {
		// Framing Error (Baudrate uyuşmazlığı, GND eksikliği veya parazit)
		__HAL_UART_CLEAR_FEFLAG(huart);
	}
	if (error_code & HAL_UART_ERROR_NE) {
		// Noise Error (Gürültü - RX pini havada kalmış)
		__HAL_UART_CLEAR_NEFLAG(huart);
	}

	// Hata durumunda DMA ve UART kilitlenir. Yeniden başlatmamız ŞARTTIR!
	HAL_UART_AbortReceive(huart);
	HAL_UARTEx_ReceiveToIdle_DMA(RS422_huartx, rs422_rx_buffer, RS422_RX_BUFFER_SIZE);
	__HAL_DMA_DISABLE_IT(RS422_huartx->hdmarx, DMA_IT_HT);

}
