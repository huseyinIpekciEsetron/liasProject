#include "ethernet_w5500.h"
#include <string.h>

static SPI_HandleTypeDef *Hspi = NULL;

uint8_t ARM_bufSize[8]       = {2,2,2,2,0,0,0,0};
bool eth_connection_state[8] = {0}; // 8 Soket için durum tutucu

// Hedef IP Atamaları
uint8_t TARGET_1_IP[4] = {192, 168, 71, 1};
uint8_t TARGET_2_IP[4] = {192, 168, 71, 2};

uint8_t temp_ethernet_ip[4] = {0};
uint16_t temp_port = 0;

// MPU ve DMA Adresleri
#define DMA_BUF_SIZE 2048
__attribute__((section(".ram_d2"), aligned(32)))
static uint8_t spi_dma_tx_buf[DMA_BUF_SIZE];

__attribute__((section(".ram_d2"), aligned(32)))
static uint8_t spi_dma_rx_buf[DMA_BUF_SIZE];
#define SPI_BURST_TIMEOUT_MS  20U

volatile Eth_Status_t eth_status = ETH_ERR_NO_CHIP;

// SPI Callback Fonksiyonları
static void cs_sel() { HAL_GPIO_WritePin(ETH_SCSn_GPIO_Port, ETH_SCSn_Pin, GPIO_PIN_RESET); }
static void cs_desel() { HAL_GPIO_WritePin(ETH_SCSn_GPIO_Port, ETH_SCSn_Pin, GPIO_PIN_SET); }
static uint8_t spi_rb(void) { uint8_t rbuf; HAL_SPI_Receive(Hspi, &rbuf, 1, 100); return rbuf; }
static void spi_wb(uint8_t b) { HAL_SPI_Transmit(Hspi, &b, 1, 100); }

static void spi_wb_burst(uint8_t* pBuf, uint16_t len) {
	if (len > DMA_BUF_SIZE) len = DMA_BUF_SIZE;
	memcpy(spi_dma_tx_buf, pBuf, len);

	if (HAL_SPI_Transmit_DMA(Hspi, spi_dma_tx_buf, len) != HAL_OK) {
		eth_status = ETH_ERR_SPI;
		return;
	}

	uint32_t t0 = HAL_GetTick();
	while (HAL_SPI_GetState(Hspi) != HAL_SPI_STATE_READY)
	{
		if ((HAL_GetTick() - t0) > SPI_BURST_TIMEOUT_MS) {
			HAL_SPI_Abort(Hspi);
			eth_status = ETH_ERR_SPI;
			return;
		}
	}
}

static void spi_rb_burst(uint8_t* pBuf, uint16_t len) {
    if(len > DMA_BUF_SIZE) len = DMA_BUF_SIZE;

    if(HAL_SPI_Receive_DMA(Hspi, spi_dma_rx_buf, len) != HAL_OK) {
    	eth_status = ETH_ERR_SPI;
		return;
    }

    uint32_t t0 = HAL_GetTick();
    while(HAL_SPI_GetState(Hspi) != HAL_SPI_STATE_READY)
    {
    	if ((HAL_GetTick() - t0) > SPI_BURST_TIMEOUT_MS) {
			HAL_SPI_Abort(Hspi);
			eth_status = ETH_ERR_SPI;
			return;
		}
    }
    memcpy(pBuf, spi_dma_rx_buf, len);
}

/* ===================================================================== */
/* 1. DONANIM BAŞLATMA                                                   */
/* ===================================================================== */
Eth_Status_t  Ethernet_Init(SPI_HandleTypeDef *spi)
{
    Hspi = spi;
    HAL_GPIO_WritePin(ETH_PMODE0_GPIO_Port, ETH_PMODE0_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(ETH_PMODE1_GPIO_Port, ETH_PMODE1_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(ETH_PMODE2_GPIO_Port, ETH_PMODE2_Pin, GPIO_PIN_SET);

    HAL_GPIO_WritePin(ETH_RSTn_GPIO_Port, ETH_RSTn_Pin, GPIO_PIN_RESET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(ETH_RSTn_GPIO_Port, ETH_RSTn_Pin, GPIO_PIN_SET);
    HAL_Delay(100);

    reg_wizchip_cs_cbfunc(cs_sel, cs_desel);
    reg_wizchip_spi_cbfunc(spi_rb, spi_wb);
    reg_wizchip_spiburst_cbfunc(spi_rb_burst, spi_wb_burst);
    HAL_Delay(300);

    if(wizchip_init(ARM_bufSize, ARM_bufSize) != 0)
    {
    	eth_status = ETH_ERR_NO_CHIP; // etherneth başlatılamadı
		return eth_status;
    }
    wiz_NetInfo netInfo = {
		.mac  = STM32_MAC_ADDR,
		.ip   = STM32_IP_ADDR,
		.sn   = STM32_SUBNET,
		.gw   = STM32_GATEWAY
	};
	wizchip_setnetinfo(&netInfo);
	HAL_Delay(10);

	wiz_NetTimeout eth_timeout = { .retry_cnt = 1, .time_100us = 100 };
	wizchip_setnetmode(NM_FORCEARP);
	wizchip_settimeout(&eth_timeout);
	HAL_Delay(100);
	eth_status = ETH_OK;
	return eth_status;
}

/* ===================================================================== */
/* 2. TCP FONKSİYONLARI                                                  */
/* ===================================================================== */
void TCP_Server_Process(uint8_t socket_num, uint16_t port, uint8_t *rx_buffer)
{
    uint8_t socket_status = getSn_SR(socket_num);

    switch (socket_status)
    {
        case SOCK_CLOSED:
            socket(socket_num, Sn_MR_TCP, port, 0x00);
            break;
        case SOCK_INIT:
            listen(socket_num);
            break;
        case SOCK_ESTABLISHED:
            if (getSn_IR(socket_num) & Sn_IR_CON) {
                setSn_IR(socket_num, Sn_IR_CON);
            }
            uint16_t size = getSn_RX_RSR(socket_num);
            if (size > 0)
            {
                if (size > 99) size = 99; // 100 byte dizi için taşma koruması
                recv(socket_num, rx_buffer, size);
                rx_buffer[size] = '\0';
            }
            break;
        case SOCK_CLOSE_WAIT:
            disconnect(socket_num);
            break;
    }
}

bool TCP_Send_Data(uint8_t socket_num, uint8_t *tx_buffer, uint16_t data_len)
{
    if(getSn_SR(socket_num) == SOCK_ESTABLISHED) {
        send(socket_num, tx_buffer, data_len);
        return true;
    }
    return false;
}

/* ===================================================================== */
/* 3. UDP FONKSİYONLARI                                                  */
/* ===================================================================== */
void UDP_Socket_Init(uint8_t socket_num, uint16_t port)
{
    socket(socket_num, Sn_MR_UDP, port, SF_IO_NONBLOCK);
}

int16_t UDP_Receive_Data(uint8_t socket_num, uint8_t *rx_buffer, uint16_t buffer_len)
{
    uint16_t rec_buffer_size = 0;
    int16_t len = 0;

    getsockopt(socket_num, SO_RECVBUF, &rec_buffer_size);
    if(rec_buffer_size > 0)
    {
        len = recvfrom(socket_num, rx_buffer, buffer_len, temp_ethernet_ip, &temp_port);
    }
    return len;
}

void UDP_Send_Data(uint8_t socket_num, uint8_t *tx_buffer, uint16_t data_len, uint8_t *ip_addr, uint16_t dest_port)
{
    sendto(socket_num, tx_buffer, data_len, ip_addr, dest_port);
}


/* ===================================================================== */
/* 4. DONANIM DURUM KONTROLLERİ (HOT-PLUG)                               */
/* ===================================================================== */
bool Ethernet_Check_Link_Status(void)
{
    static uint8_t previous_link_status = 1;
    uint8_t current_link_status = wizphy_getphylink();

    // Eğer kablo ŞU AN koptuysa ve az önce takılıysa...
    if (current_link_status == PHY_LINK_OFF && previous_link_status != PHY_LINK_OFF)
    {
        // 8 donanımsal soketi tara
        for(int i = 0; i < 8; i++)
        {
            // getSn_MR(i) fonksiyonu o soketin modunu okur.
            // Eğer soket TCP modunda (Sn_MR_TCP) çalışıyorsa onu kapat.
            // UDP modundaysa (veya kapalıysa) hiçbir şey yapma!
            if((getSn_MR(i) & 0x0F) == Sn_MR_TCP)
            {
                close(i);
            }
        }
    }

    previous_link_status = current_link_status;
    return previous_link_status;
}
