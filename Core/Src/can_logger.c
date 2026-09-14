/*
 * can_logger.c
 *
 *  Created on: Jul 27, 2026
 *      Author: HUSEYIN
 */


#include "can_logger.h"
#include "main.h"

#define CAN_LOG_QUEUE_SIZE 64 // Kuyrukta en fazla 64 log birikebilir

typedef struct {
    uint8_t data[8];
} CanLogMsg_t;

static FDCAN_HandleTypeDef *logger_fdcan;
static FDCAN_TxHeaderTypeDef logger_TxHeader;

static CanLogMsg_t log_queue[CAN_LOG_QUEUE_SIZE];
static volatile uint16_t log_head = 0;
static volatile uint16_t log_tail = 0;

void CAN_Logger_Init(FDCAN_HandleTypeDef *hfdcan)
{
    logger_fdcan = hfdcan;

    // 1. FDCAN Modülünü Başlat
	if (HAL_FDCAN_Start(logger_fdcan) != HAL_OK) {
	  Error_Handler();
	}

    // CAN TX Başlığını (Header) Klasik CAN standardına göre hazırla
    logger_TxHeader.Identifier = 0x123; // Sisteminizin Log İletişim ID'si
    logger_TxHeader.IdType = FDCAN_STANDARD_ID;
    logger_TxHeader.TxFrameType = FDCAN_DATA_FRAME;
    logger_TxHeader.DataLength = FDCAN_DLC_BYTES_8; // 8 Byte
    logger_TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    logger_TxHeader.BitRateSwitch = FDCAN_BRS_OFF;
    logger_TxHeader.FDFormat = FDCAN_CLASSIC_CAN;
    logger_TxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    logger_TxHeader.MessageMarker = 0;

    log_head = 0;
    log_tail = 0;
}

void CAN_Log_Add(CanLogType_t type, uint8_t byte1, uint8_t byte2, uint8_t byte3,
                 uint8_t byte4, uint8_t byte5, uint8_t byte6, uint8_t byte7)
{
	uint32_t primask = __get_PRIMASK();
	__disable_irq();

    // Kuyrukta bir sonraki pozisyonu hesapla
    uint16_t next_head = (log_head + 1) % CAN_LOG_QUEUE_SIZE;

    // Eğer kuyruk dolu değilse logu ekle
    // (Doluysa log düşer/atlanır, ancak sistem kilitlenmez! Fail-Safe özelliği)
    if (next_head != log_tail)
    {
        log_queue[log_head].data[0] = (uint8_t)type;
        log_queue[log_head].data[1] = byte1;
        log_queue[log_head].data[2] = byte2;
        log_queue[log_head].data[3] = byte3;
        log_queue[log_head].data[4] = byte4;
        log_queue[log_head].data[5] = byte5;
        log_queue[log_head].data[6] = byte6;
        log_queue[log_head].data[7] = byte7;

        log_head = next_head; // Yazma noktasını ilerlet
    }

    __set_PRIMASK(primask);
}

void CAN_Logger_Process(void)
{
	if (log_head != log_tail)
	{
		if (HAL_FDCAN_GetTxFifoFreeLevel(logger_fdcan) > 0)
		{
			// Okuma esnasında dizinin veri tutarlılığını garantiye al
			uint32_t primask = __get_PRIMASK();
			__disable_irq();

			uint16_t current_tail = log_tail;

			__set_PRIMASK(primask);

			if (HAL_FDCAN_AddMessageToTxFifoQ(logger_fdcan, &logger_TxHeader, log_queue[current_tail].data) == HAL_OK)
			{
				primask = __get_PRIMASK();
				__disable_irq();

				log_tail = (log_tail + 1) % CAN_LOG_QUEUE_SIZE;

				__set_PRIMASK(primask);
			}
		}
	}
}
