/*
 * uart2_driver.c
 *
 *  Created on: Jul 13, 2026
 *      Author: HUSEYIN
 */


#include "uart2_driver.h"
#include <string.h>

extern CRC_HandleTypeDef hcrc;          /* main.c'de tanimli, su ana kadar kullanilmiyordu */

#define BMB_FRAME_LEN   64U

/* ISR ile ana dongu arasindaki tek paylasim noktasi.
 * ISR yalnizca buraya yazar; dogrulama ana donguye ait. */
static volatile uint8_t  rawFrame[BMB_FRAME_LEN];
static volatile bool     rawFrameReady = false;
static volatile uint32_t rawFrameDropped = 0U;   /* teshis sayaci */

// MPU ayarlarında Non-Cacheable yaptığın SRAM alanına yerleştiriyoruz
#if defined ( __GNUC__ )
    __attribute__((section(".ram_d2"), aligned(32))) uint8_t uart2_rx_buffer[UART2_RX_BUFFER_SIZE];
    __attribute__((section(".ram_d2"), aligned(32))) uint8_t uart2_tx_buffer[UART2_TX_BUFFER_SIZE];
#else
    __attribute__((aligned(32))) uint8_t uart2_rx_buffer[UART2_RX_BUFFER_SIZE];
    __attribute__((aligned(32))) uint8_t uart2_tx_buffer[UART2_TX_BUFFER_SIZE];
#endif

// UART2 Handle
UART_HandleTypeDef* UART2_huartx;

static BMB_RxPacket_t Validated_Rx_Packet = {0};
static BMB_TxPacket_t bmb_tx_packet = {0};

static uint8_t bmb_timestamp = 0;
static bool new_bmb_data_flag = false;

// TX Paketi için dinamik değişkenler
static uint8_t current_tx_power_ready = 0x00;
static uint8_t current_tx_sis_blast[16] = {0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55};
static uint8_t current_tx_frag_blast[16] = {0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55};


/**
 * @brief Yazılımsal CRC32 Hesaplama Fonksiyonu
 */
static uint32_t Calculate_Software_CRC32(uint8_t *data, uint32_t length)
{
    uint32_t crc = 0xFFFFFFFF;  // STM32 Varsayılan Başlangıç Değeri (Init Value)
    uint32_t poly = 0x04C11DB7; // STM32 Varsayılan Ethernet Polinomu

    for (uint32_t i = 0; i < length; i++)
    {
        // Gelen 8-bitlik veriyi (Byte), 32-bitlik CRC değişkeninin en anlamlı byte'ına XOR'luyoruz
        crc ^= ((uint32_t)data[i] << 24);

        // Her byte için 8 bitlik kaydırma (shift) işlemi
        for (uint8_t bit = 0; bit < 8; bit++)
        {
            if (crc & 0x80000000) // Eğer en sol bit 1 ise
            {
                crc = (crc << 1) ^ poly; // Kaydır ve polinom ile XOR'la
            }
            else
            {
                crc = (crc << 1); // Sadece kaydır
            }
        }
    }

    return crc; // STM32 varsayılan ayarında Final XOR yoktur, doğrudan döndürülür
}

/**
 * @brief UART2 DMA ReceiveToIdle Başlatma (Normal Mod)
 */
bool UART2_Init(UART_HandleTypeDef *huartx)
{
    UART2_huartx = huartx;

    // Eğer başlatma başarısız olursa (HAL_OK dönmezse) Error Handler'a düşsün ki hatayı görelim!
    if (HAL_UARTEx_ReceiveToIdle_DMA(UART2_huartx, uart2_rx_buffer, UART2_RX_BUFFER_SIZE) != HAL_OK)
    {
        return false;
    }

    __HAL_DMA_DISABLE_IT(UART2_huartx->hdmarx, DMA_IT_HT);
    return true;
}

/**
 * @brief UART2 Üzerinden DMA ile Veri Gönderme (Normal Mod)
 */
void UART2_Send_Data(uint8_t *data, uint16_t length)
{
    if(length == 0 || length > UART2_TX_BUFFER_SIZE) return;

    uint32_t t0 = HAL_GetTick();
    // 1. Bir önceki gönderimin bitmesini bekle
    while (UART2_huartx->gState != HAL_UART_STATE_READY) {
    	if ((HAL_GetTick() - t0) > 5U) {
			/* Onceki gonderim takildi - iptal et ve bu cevrimi atla.
			 * 50 Hz'de yeniden denenecek. */
			HAL_UART_AbortTransmit(UART2_huartx);
			return;
		}
    }

    // 2. Gönderilecek veriyi TX buffer'ına kopyala (Şu an Cache'de)
    memcpy(uart2_tx_buffer, data, length);

    // =======================================================
    // 3. HAYAT KURTARAN SATIR: CACHE CLEAN (TEMİZLEME)
    // İşlemcinin önbelleğindeki (Cache) taze verileri zorla
    // Fiziksel RAM'e itiyoruz ki DMA doğru veriyi çekebilsin!
    // =======================================================
    SCB_CleanDCache_by_Addr((uint32_t *)uart2_tx_buffer, length);

    // 4. TX DMA Normal modda olduğu için gönderimi başlatıyoruz
    HAL_UART_Transmit_DMA(UART2_huartx, uart2_tx_buffer, length);
}

/**
 * @brief Gönderim Tamamlandığında Çalışan Kesme
 */
void UART2_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        // Gönderim bitti
    }
}

/**
 * @brief Veri Alımı (Idle) Tamamlandığında Çalışan Kesme
 */
void UART2_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	if (huart->Instance == USART2)
	{
		/* Tampon .ram_d2'de ve non-cacheable oldugu icin invalidate
		 * artik gereksiz; yine de zararsiz olsun diye biraktik.
		 * Adim 2 dogrulandiktan sonra silinebilir. */
		SCB_InvalidateDCache_by_Addr((uint32_t *)uart2_rx_buffer, UART2_RX_BUFFER_SIZE);

		if (Size == BMB_FRAME_LEN)
		{
			if (rawFrameReady) {
				rawFrameDropped++;      /* ana dongu yetisememis - teshis icin say */
			}
			memcpy((void *)rawFrame, uart2_rx_buffer, BMB_FRAME_LEN);
			rawFrameReady = true;       /* SON yazilan: yayinlama noktasi */
		}

		HAL_UARTEx_ReceiveToIdle_DMA(UART2_huartx, uart2_rx_buffer, UART2_RX_BUFFER_SIZE);
		__HAL_DMA_DISABLE_IT(UART2_huartx->hdmarx, DMA_IT_HT);
	}
}

/**
 * @brief UART Hatası Meydana Geldiğinde Çalışan Kesme
 */
void UART2_ErrorCallback(UART_HandleTypeDef *huart)
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
	HAL_UARTEx_ReceiveToIdle_DMA(UART2_huartx, uart2_rx_buffer, UART2_RX_BUFFER_SIZE);
	__HAL_DMA_DISABLE_IT(UART2_huartx->hdmarx, DMA_IT_HT);

}



/**
 * @brief Dinamik Verilerle BMB Test Paketini Doldurur, CRC hesaplar ve Gönderir
 */
void UART2_Send_BMB_Test_Packet(void)
{
    // 1. Sabit Değerleri (Header, Footer, Reserved) Doldur
    bmb_tx_packet.header[0] = 'E';
    bmb_tx_packet.header[1] = 'S';
    bmb_tx_packet.header[2] = 'E';
    bmb_tx_packet.header[3] = 0x02;

    bmb_tx_packet.footer[0] = 'O';
    bmb_tx_packet.footer[1] = 'N';

    memset(bmb_tx_packet.reserved, 0, 20); // 20 byte rezerve alanı sıfırla

    // 2. Dinamik Değerleri Doldur (Timestamp)
    bmb_tx_packet.timestamp = bmb_timestamp;
    bmb_timestamp++;
    if(bmb_timestamp > 200) bmb_timestamp = 0;

    // 3. C++ Tarafından Seçilmiş SİS, FRAG ve POWER Verilerini Doldur
    for(int i = 0; i < 16; i++) {
        bmb_tx_packet.sis_status[i]  = current_tx_sis_blast[i];
        bmb_tx_packet.frag_status[i] = current_tx_frag_blast[i];
    }
    bmb_tx_packet.power_ready = current_tx_power_ready;

    // Yapımızı byte bazlı manipüle edebilmek için array pointer'ına çeviriyoruz
    uint8_t *packet_array = (uint8_t *)&bmb_tx_packet;

    // 4. YAZILIMSAL CRC32 HESAPLAMA (İlk 60 Byte)
    uint32_t Calculated_CRC = Calculate_Software_CRC32(packet_array, 60);

    // 5. CRC Paket Yerleşimi (Manuel Big-Endian Düzeni)
    packet_array[63] = Calculated_CRC & 0xFF;
    packet_array[62] = (uint8_t)((Calculated_CRC >> 8) & 0xFF);
    packet_array[61] = (uint8_t)((Calculated_CRC >> 16) & 0xFF);
    packet_array[60] = (uint8_t)((Calculated_CRC >> 24) & 0xFF);

    // 6. Paketi DMA ile UART2 üzerinden Gönder (Tam 64 Byte)
    UART2_Send_Data(packet_array, sizeof(BMB_TxPacket_t));
}


// C++ ARAYÜZÜ İÇİN KÖPRÜ (GETTER)
// Model.cpp bu fonksiyonu çağırdığında, kasadaki veriyi ona güvenle kopyalarız (extern kullanmadan!)
void BMB_Get_Latest_Rx_Data(BMB_RxPacket_t *out_data)
{
    if(out_data != NULL) {
        // İstenirse buraya bir Mutex/Semaphor eklenebilir, şimdilik direkt kopyalıyoruz
        memcpy(out_data, &Validated_Rx_Packet, sizeof(BMB_RxPacket_t));
    }
}

// C++ Tarafı "Yeni bir şey var mı?" diye soracak. Varsa true dönüp bayrağı indireceğiz.
bool BMB_Check_New_Data(void)
{
    if (new_bmb_data_flag) {
        new_bmb_data_flag = false;
        return true;
    }
    return false;
}

// C++ tarafından Ateşleme Komutlarını belirlemek için
void BMB_Set_Tx_Command(bool power_ready, uint16_t sis_mask, uint16_t frag_mask)
{
    current_tx_power_ready = power_ready ? 0xAA : 0x00;

    for(int i = 0; i < 16; i++) {
        // Eğer o bit 1 ise (Seçilmişse) 0xAA yolla, değilse 0x55 (Ateşleme)
        current_tx_sis_blast[i]  = ((sis_mask & (1 << i)) != 0)  ? 0xAA : 0x55;
        current_tx_frag_blast[i] = ((frag_mask & (1 << i)) != 0) ? 0xAA : 0x55;
    }
}

/**
 * @brief Ham cerceveyi dogrular ve gecerliyse yayinlar.
 *        ANA DONGUDEN cagrilir, ISR'dan DEGIL.
 */
void UART2_ProcessRxFrame(void)
{
    if (!rawFrameReady) return;

    uint8_t local[BMB_FRAME_LEN];

    /* Kisa kritik bolum: 64 bayt kopyalarken ISR araya girmesin.
     * ~100 ns surer, kesme gecikmesine etkisi ihmal edilebilir. */
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    memcpy(local, (const void *)rawFrame, BMB_FRAME_LEN);
    rawFrameReady = false;
    __set_PRIMASK(primask);

    /* --- Bundan sonrasi tamamen ana dongu baglaminda --- */
    BMB_RxPacket_t *paket = (BMB_RxPacket_t *)local;

    if (!(paket->header[0] == 'E' && paket->header[1] == 'S' &&
          paket->header[2] == 'E' && paket->header[3] == 0x01 &&
          paket->footer[0] == 'O' && paket->footer[1] == 'N')) {
        return;
    }

    /* Donanim CRC birimi - yazilimsal 480 iterasyon yerine ~60 cevrim */
    uint32_t hesaplanan = HAL_CRC_Calculate(&hcrc, (uint32_t *)local, 60U);

    uint32_t gelen = ((uint32_t)local[60] << 24) |
                     ((uint32_t)local[61] << 16) |
                     ((uint32_t)local[62] <<  8) |
                     ((uint32_t)local[63]);

    if (hesaplanan == gelen)
    {
        memcpy(&Validated_Rx_Packet, paket, sizeof(BMB_RxPacket_t));
        new_bmb_data_flag = true;
        last_rx_time = HAL_GetTick();   /* CBIT'in iletisim izlemesi icin */
    }
}
