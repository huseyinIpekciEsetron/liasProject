/*
 * ethernet_w5500.h
 *
 * Created on: Oct 15, 2025
 * Author: Huseyin
 */

#ifndef INC_ETHERNET_W5500_H_
#define INC_ETHERNET_W5500_H_

#include <stdbool.h>
#include "main.h"
#include "wizchip_conf.h"
#include "socket.h"

/* ===================================================================== */
/* KULLANICI AYARLARI (USER SETTINGS)                                 */
/* ===================================================================== */

// 1. KENDİ CİHAZIMIZIN (STM32) AĞ BİLGİLERİ
#define STM32_MAC_ADDR 		{0xC4, 0x93, 0x01, 0x01, 0x02, 0x02}
#define STM32_IP_ADDR  		{192, 168, 71, 3}
#define STM32_SUBNET   		{255, 255, 255, 0}
#define STM32_GATEWAY  		{192, 168, 71, 1}

// 2. HEDEF 1 BİLGİLERİ
#define TARGET_1_PORT 		54307
extern uint8_t TARGET_1_IP[4];

// 3. HEDEF 2 BİLGİLERİ
#define TARGET_2_PORT 		50000
extern uint8_t TARGET_2_IP[4];

/* ===================================================================== */
/* SOKET TANIMLAMALARI (Maksimum 8 Soket)                                */
/* ===================================================================== */
enum Connection_Socket
{
	TARGET_1_SOCKET = 0,
	TARGET_2_SOCKET = 1,

};

extern bool eth_connection_state[];

/* ===================================================================== */
/* KÜTÜPHANE FONKSİYONLARI                                               */
/* ===================================================================== */

// Donanım Başlatma
void Ethernet_Init(SPI_HandleTypeDef *spi);
bool Ethernet_Check_Link_Status(void);

// TCP Fonksiyonları
void TCP_Server_Process(uint8_t socket_num, uint16_t port, uint8_t *rx_buffer);
bool TCP_Send_Data(uint8_t socket_num, uint8_t *tx_buffer, uint16_t data_len);

// UDP Fonksiyonları
void UDP_Socket_Init(uint8_t socket_num, uint16_t port);
int16_t UDP_Receive_Data(uint8_t socket_num, uint8_t *rx_buffer, uint16_t buffer_len);
void UDP_Send_Data(uint8_t socket_num, uint8_t *tx_buffer, uint16_t data_len, uint8_t *ip_addr, uint16_t dest_port);

#endif /* INC_ETHERNET_W5500_H_ */
