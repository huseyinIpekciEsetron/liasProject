/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "app_touchgfx.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ethernet_w5500.h"
#include <stdio.h>
#include <string.h>
#include "hc165_driver.h"
#include "lp5036_led_driver.h"
#include "rs422_driver.h"
#include "buzzer_driver.h"
#include "uart2_driver.h"
#include "rtc.h"
#include "can_logger.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define SDRAM_START_ADDRESS 	0xD0000000
#define REFRESH_COUNT           ((uint32_t)3104)   /* SDRAM refresh counter */
#define SDRAM_TIMEOUT           ((uint32_t)0xFFFF)
#define TEST_SIZE  				10000 // 10.000 adet 32-bit (40 KB) veri test edilecek

/**
  * @brief  FMC SDRAM Mode definition register defines
  */
// SDRAM Mode Register Standart Ayarları
#define SDRAM_MODEREG_BURST_LENGTH_1             ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL      ((uint16_t)0x0000)
#define SDRAM_MODEREG_CAS_LATENCY_2              ((uint16_t)0x0020) // CAS 2 genelde en hızlı/güvenli olandır
#define SDRAM_MODEREG_OPERATING_MODE_STANDARD    ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE     ((uint16_t)0x0200)

// A3 VE A5 PİNLERİNİ YAZILIMSAL OLARAK YER DE ?İ ?TİREN MAKRO */
#define SWAP_A3_A5(val) ( ((val) & ~((1<<3) | (1<<5))) | (((val) & (1<<3)) << 2) | (((val) & (1<<5)) >> 2) )

#define FREQ_TO_MS(hz)  (1000 / (hz))
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

CRC_HandleTypeDef hcrc;

DAC_HandleTypeDef hdac1;

DMA2D_HandleTypeDef hdma2d;

FDCAN_HandleTypeDef hfdcan2;

I2C_HandleTypeDef hi2c1;

IWDG_HandleTypeDef hiwdg1;

LTDC_HandleTypeDef hltdc;

RTC_HandleTypeDef hrtc;

SPI_HandleTypeDef hspi4;
DMA_HandleTypeDef hdma_spi4_tx;
DMA_HandleTypeDef hdma_spi4_rx;

TIM_HandleTypeDef htim4;
TIM_HandleTypeDef htim7;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart1_tx;
DMA_HandleTypeDef hdma_usart2_rx;
DMA_HandleTypeDef hdma_usart2_tx;

SDRAM_HandleTypeDef hsdram1;

/* USER CODE BEGIN PV */

/* Backup SRAM veya RTC backup register'a yazilabilir; simdilik
   noinit bir degisken yeterli. */
__attribute__((section(".noinit"))) volatile uint32_t g_fault_marker;

uint8_t udp_rx_buffer[100]; // Hedef 1'den (UDP) gelenler buraya düşecek
uint8_t tcp_rx_buffer[100]; // Hedef 2'den (TCP) gelenler buraya düşecek

uint8_t udp_tx_buffer[50] = "Merhaba Hedef 1 (UDP)!\r\n";
uint8_t tcp_tx_buffer[50] = "Merhaba Hedef 2 (TCP)!\r\n";

bool hotPlug = 0;

// ========================================================
// YAZILIMSAL GÖREV YÖNETİCİSİ (TASK SCHEDULER) ZAMANLAYICILARI
// ========================================================
uint32_t task_time_bmb_uart = 0;
uint32_t task_time_can_log  = 0;
uint32_t task_time_eth_tx   = 0;
uint32_t task_time_eth_rx   = 0;
uint32_t task_time_buttons  = 0;



// ========================================================
// GELİ�?Mİ�? TEHDİT SİMÜLATÖRÜ (Live Expressions İçin)
// ========================================================
#define MAX_DBG_THREATS 5 // Aynı anda test edilecek maksimum hedef sayısı

volatile uint8_t  dbg_threat_count = 1; // Başlangıçta 2 hedef göndersin

// Aşağıdaki değişkenleri Live Expressions'a ekleyip yanındaki OK işaretinden genişletin:
// Index 0 -> 1. Hedef, Index 1 -> 2. Hedef ...
volatile uint8_t  dbg_threat_ageout[MAX_DBG_THREATS] = {0,   0,   0,   0,   0}; // 1 Yaparsan Söner!
volatile uint16_t dbg_threat_angles[MAX_DBG_THREATS] = {30,  70,  150, 220, 310}; // Dereceler
volatile uint8_t  dbg_threat_classes[MAX_DBG_THREATS]= {1,   2,   4,   0,   1}; // 0:SRCH, 1:LRF, 2:LD, 4:LBR
volatile uint8_t  dbg_threat_bands[MAX_DBG_THREATS]  = {2,   3,   1,   2,   3}; // 1:Yeşil, 2:Turuncu, 3:Mor
volatile uint8_t  dbg_threat_prios[MAX_DBG_THREATS]  = {5,   12,  1,   20,  10}; // Öncelik

// --- ZAMAN DE�?İ�?KENLERİ ---
volatile uint8_t dbg_send_time        = 1;
volatile uint8_t dbg_time_hour        = 0;
volatile uint8_t dbg_time_minute      = 0;
volatile uint8_t dbg_time_second      = 0;
volatile uint16_t dbg_time_dayOfYear  = 1;
volatile uint16_t dbg_time_year       = 2026;

uint32_t lattime = 0;

volatile uint32_t dbg_loopMaxMs = 0;
volatile uint32_t dbg_loopCount = 0;
volatile uint32_t dbg_bmbFps    = 0;   /* BMB cerceve / saniye */
volatile uint32_t dbg_loopFps   = 0;   /* ana dongu turu / saniye */
volatile uint32_t dbg_loopMax1s = 0;   /* SON 1 saniyedeki en kotu tur */
volatile uint32_t dbg_rawTotal = 0;
volatile uint32_t dbg_loopMaxEver = 0;   /* hic otomatik sifirlanmaz */
volatile uint32_t dbg_loopOver20  = 0;   /* 20 ms'yi asan tur sayisi */
volatile uint32_t dbg_ltdcUnderrun = 0;
volatile uint32_t dbg_ltdcXferErr  = 0;

volatile uint8_t dbg_send_sys_status = 0;
// İşlemci Hataları [0]: Byte 7-8, [1]: Byte 9-10, [2]: Byte 11-12
volatile uint16_t dbg_sys_proc_faults[3] = {0, 0, 0};

// Sensör 1, 2, 3, 4 için MSB (Bant Hataları) ve LSB (Voltaj/Sıcaklık Hataları)
volatile uint16_t dbg_sys_sens_faults_msb[4] = {0, 0, 0, 0};
volatile uint16_t dbg_sys_sens_faults_lsb[4] = {0, 0, 0, 0};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_FMC_Init(void);
static void MX_LTDC_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_CRC_Init(void);
static void MX_DMA2D_Init(void);
static void MX_DAC1_Init(void);
static void MX_SPI4_Init(void);
static void MX_FDCAN2_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM4_Init(void);
static void MX_RTC_Init(void);
static void MX_IWDG1_Init(void);
static void MX_TIM7_Init(void);
/* USER CODE BEGIN PFP */

uint8_t SDRAM_Health_Test(void)
{
    volatile uint32_t *sdram_ptr = (volatile uint32_t *)SDRAM_START_ADDRESS;
    uint32_t test_pattern = 0xAA55AA55; // Elektronikte klasik test desenidir (101010...)

    // 1. A�?AMA: SDRAM'e Veri Yaz
    for (int i = 0; i < TEST_SIZE; i++)
    {
        sdram_ptr[i] = test_pattern + i; // Her adrese farklı bir sayı yaz
    }

    // 2. A�?AMA: SDRAM'den Veri Oku ve Kontrol Et
    for (int i = 0; i < TEST_SIZE; i++)
    {
        if (sdram_ptr[i] != (test_pattern + i))
        {
            return 0; // HATA! Veri bozulmuş veya yazılamamış. SDRAM ÇALI�?MIYOR.
        }
    }

    return 1; // BA�?ARILI! SDRAM kusursuz çalışıyor.
}

//static void BSP_SDRAM_Initialization_Sequence(SDRAM_HandleTypeDef *hsdram, FMC_SDRAM_CommandTypeDef *Command);
void SDRAM_Initialization_Sequence(SDRAM_HandleTypeDef *hsdram);

void Set_LCD_Brightness(uint8_t value);

extern void TLUS_Donanimdan_Gelen_Veri(uint8_t* buffer, uint16_t length);

void Debug_Data_Injector(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* Enable the CPU Cache */

  /* Enable I-Cache---------------------------------------------------------*/
  SCB_EnableICache();

  /* Enable D-Cache---------------------------------------------------------*/
  SCB_EnableDCache();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /*-- DEBUGDA IWDG RESET ATMASIN DİYE --*/
#ifdef DEBUG
    // Doğrudan debug ayarlarını çağırabilirsiniz
    HAL_DBGMCU_EnableDBGSleepMode();
    HAL_DBGMCU_EnableDBGStopMode();

    // Watchdog 1'i breakpoint'lerde dondur
    __HAL_DBGMCU_FREEZE_IWDG1();
#endif

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_FMC_Init();
  MX_LTDC_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_CRC_Init();
  MX_DMA2D_Init();
  MX_DAC1_Init();
  MX_SPI4_Init();
  MX_FDCAN2_Init();
  MX_I2C1_Init();
  MX_TIM4_Init();
  MX_RTC_Init();
  //MX_IWDG1_Init();
  MX_TIM7_Init();
  MX_TouchGFX_Init();
  /* USER CODE BEGIN 2 */

  __HAL_LTDC_ENABLE_IT(&hltdc, LTDC_IT_FU | LTDC_IT_TE);

  /*-- LCD Init --*/
  HAL_GPIO_WritePin(LCD_ONOFF_GPIO_Port, LCD_ONOFF_Pin, GPIO_PIN_SET);

  /*-- brightness --*/
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
  TIM4->CCR1 = 60;

  /*-- Ethernet W5500 --*/
  if (Ethernet_Init(&hspi4) == ETH_OK) {
      UDP_Socket_Init(TARGET_1_SOCKET, TARGET_1_PORT);
  }
  else {
	  /* ETH_OK degilse sistem calismaya devam eder; ekranda
	     "TLUS BAGLANTI YOK" gosterilir. */
  }

  /*-- Button Shift Register --*/
  HC165_Init();

  /*-- Led Driver --*/
  LP5036LedDriver_Init(&hi2c1);

  /*-- RS422 --*/
  RS422_Init(&huart1);

  /*-- UART2 BMB Driver--*/
  if(UART2_Init(&huart2) == false)
  {
	  // ekranda uart bağlantısının olmadığını söyle
  }

  /*-- CAN Logs --*/
  CAN_Logger_Init(&hfdcan2);

  /*-- Buzzer Init --*/
  Buzzer_Init(&hdac1, DAC_CHANNEL_1);

  /*-- RTC Init --*/
  RTC_Init(&hrtc);

  bool led_ok = false;
  for (uint8_t retry = 0; retry < 3U && !led_ok; retry++) {
      led_ok = (LP5036_SetColor(3, 255) == LP5036_OK);
  }


  /* --- buton ledlerinin başlatılması --- */
  LP5036_SetButtonLeds(LP5036_LED_DIM);


  /*-- Task Time Init --*/
  task_time_eth_rx = HAL_GetTick();
  task_time_eth_tx = HAL_GetTick();
  task_time_can_log = HAL_GetTick();
  task_time_buttons = HAL_GetTick();
  task_time_bmb_uart = HAL_GetTick();

  MX_IWDG1_Init();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  HAL_IWDG_Refresh(&hiwdg1);
    /* USER CODE END WHILE */

  MX_TouchGFX_Process();
    /* USER CODE BEGIN 3 */
  	  uint32_t current_time = HAL_GetTick(); // Sistemin o anki milisaniyesini tek sefer çek

  	static uint32_t lastLoopMs = 0;
	  if (lastLoopMs != 0U) {                     /* ilk tur artefakt, sayma */
		  uint32_t loopDt = current_time - lastLoopMs;
		  if (loopDt > dbg_loopMaxMs) dbg_loopMaxMs = loopDt;
		  if (loopDt > 20U)             dbg_loopOver20++;
	  }
	  lastLoopMs = current_time;
	  dbg_loopCount++;
	  static uint32_t rateT0 = 0, rawN0 = 0, loopN0 = 0;
	if (current_time - rateT0 >= 1000U)
	{
		rateT0        = current_time;
		dbg_bmbFps    = dbg_rawTotal  - rawN0;   rawN0  = dbg_rawTotal;
		dbg_loopFps   = dbg_loopCount - loopN0;  loopN0 = dbg_loopCount;
		dbg_loopMax1s = dbg_loopMaxMs;           /* pencereyi yayinla */
		dbg_loopMaxMs = 0;                       /* ve sifirla */
	}




  	  // =========================================================
	  // GÖREV: KABLO BA�?LANTISI KONTROLÜ (1 Hz -> 1000ms)
	  // PHY Link Status çok ağır bir işlemdir, saniyede 1 kez sorulur.
	  // =========================================================
	  static uint32_t task_time_eth_link = 0;
	  if (current_time - task_time_eth_link >= FREQ_TO_MS(1))
	  {
		  task_time_eth_link = current_time;
		  hotPlug = Ethernet_Check_Link_Status();
	  }

      // =========================================================
      // GÖREV 1: FİZİKSEL KABLO VE ETHERNET RX POLING (Örn: 100 Hz -> 10ms)
      // W5500 SPI üzerinden çalıştığı için ara ara donanıma "Veri var mı?" diye sormalıyız.
      // =========================================================
      if (current_time - task_time_eth_rx >= FREQ_TO_MS(100))
      {
          task_time_eth_rx = current_time;

          // Hedef 1 - UDP Dinleme
          int16_t udp_len = UDP_Receive_Data(TARGET_1_SOCKET, udp_rx_buffer, 100);
          if(udp_len > 0) {
              if (udp_len > 99) udp_len = 99;
              udp_rx_buffer[udp_len] = '\0';
             // TLUS_Donanimdan_Gelen_Veri(udp_rx_buffer, udp_len);
          }

          // Hedef 2 - TCP Dinleme
          TCP_Server_Process(TARGET_2_SOCKET, TARGET_2_PORT, tcp_rx_buffer);
      }

      // =========================================================
      // GÖREV 2: BMB GÜÇ KARTI UART HABERLE�?MESİ (İstenen: 50 Hz -> 20ms)
      // =========================================================
      if (current_time - task_time_bmb_uart >= FREQ_TO_MS(50))
      {
          task_time_bmb_uart = current_time;

          // Ateşleme komutları ve durum isteklerini 50Hz hızında yolla
          UART2_Send_BMB_Test_Packet();
      }
      UART2_ProcessRxFrame();

      // =========================================================
      // GÖREV 3: BUTON OKUMALARI (Örn: 50 Hz -> 20ms)
      // =========================================================
      if (current_time - task_time_buttons >= FREQ_TO_MS(50))
      {
          task_time_buttons = current_time;
          HC165_Read_State();
      }

      // =========================================================
      // GÖREV 4: CAN BUS LOG GÖNDERİMİ (Örn: 10 Hz -> 100ms)
      // ICD gelince bunu belki de "Sürekli değil, sadece log oldukça gönder"
      // şeklinde Event-Driven (Olay tabanlı) bir yapıya çevirebiliriz.
      // =========================================================
      if (current_time - task_time_can_log >= FREQ_TO_MS(10))
      {
          task_time_can_log = current_time;

          // Kuyrukta log varsa, sistemi bloke etmeden donanıma yazar
          CAN_Logger_Process();
      }

      // =========================================================


      // GÖREV 5: ETHERNET TX BİLGİ GÖNDERİMİ (Örn: 1 Hz -> 1000ms)
      // Karşıya "Ben hayattayım" veya periyodik sistem durumu yollamak için
      // =========================================================
      if (current_time - task_time_eth_tx >= FREQ_TO_MS(1))
      {
          task_time_eth_tx = current_time;

          UDP_Send_Data(TARGET_1_SOCKET, udp_tx_buffer, strlen((char*)udp_tx_buffer), TARGET_1_IP, TARGET_1_PORT);
          TCP_Send_Data(TARGET_2_SOCKET, tcp_tx_buffer, strlen((char*)tcp_tx_buffer));
      }

      Debug_Data_Injector();
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 2;
  RCC_OscInitStruct.PLL.PLLN = 64;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  hcrc.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_ENABLE;
  hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
  hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
  hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
  hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_BYTES;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

}

/**
  * @brief DAC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_DAC1_Init(void)
{

  /* USER CODE BEGIN DAC1_Init 0 */

  /* USER CODE END DAC1_Init 0 */

  DAC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN DAC1_Init 1 */

  /* USER CODE END DAC1_Init 1 */

  /** DAC Initialization
  */
  hdac1.Instance = DAC1;
  if (HAL_DAC_Init(&hdac1) != HAL_OK)
  {
    Error_Handler();
  }

  /** DAC channel OUT1 config
  */
  sConfig.DAC_SampleAndHold = DAC_SAMPLEANDHOLD_DISABLE;
  sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
  sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
  sConfig.DAC_ConnectOnChipPeripheral = DAC_CHIPCONNECT_DISABLE;
  sConfig.DAC_UserTrimming = DAC_TRIMMING_FACTORY;
  if (HAL_DAC_ConfigChannel(&hdac1, &sConfig, DAC_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DAC1_Init 2 */

  /* USER CODE END DAC1_Init 2 */

}

/**
  * @brief DMA2D Initialization Function
  * @param None
  * @retval None
  */
static void MX_DMA2D_Init(void)
{

  /* USER CODE BEGIN DMA2D_Init 0 */

  /* USER CODE END DMA2D_Init 0 */

  /* USER CODE BEGIN DMA2D_Init 1 */

  /* USER CODE END DMA2D_Init 1 */
  hdma2d.Instance = DMA2D;
  hdma2d.Init.Mode = DMA2D_M2M;
  hdma2d.Init.ColorMode = DMA2D_OUTPUT_RGB565;
  hdma2d.Init.OutputOffset = 0;
  hdma2d.LayerCfg[1].InputOffset = 0;
  hdma2d.LayerCfg[1].InputColorMode = DMA2D_INPUT_RGB565;
  hdma2d.LayerCfg[1].AlphaMode = DMA2D_NO_MODIF_ALPHA;
  hdma2d.LayerCfg[1].InputAlpha = 0;
  hdma2d.LayerCfg[1].AlphaInverted = DMA2D_REGULAR_ALPHA;
  hdma2d.LayerCfg[1].RedBlueSwap = DMA2D_RB_REGULAR;
  hdma2d.LayerCfg[1].ChromaSubSampling = DMA2D_NO_CSS;
  if (HAL_DMA2D_Init(&hdma2d) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_DMA2D_ConfigLayer(&hdma2d, 1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DMA2D_Init 2 */

  /* USER CODE END DMA2D_Init 2 */

}

/**
  * @brief FDCAN2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_FDCAN2_Init(void)
{

  /* USER CODE BEGIN FDCAN2_Init 0 */

  /* USER CODE END FDCAN2_Init 0 */

  /* USER CODE BEGIN FDCAN2_Init 1 */

  /* USER CODE END FDCAN2_Init 1 */
  hfdcan2.Instance = FDCAN2;
  hfdcan2.Init.FrameFormat = FDCAN_FRAME_CLASSIC;
  hfdcan2.Init.Mode = FDCAN_MODE_NORMAL;
  hfdcan2.Init.AutoRetransmission = ENABLE;
  hfdcan2.Init.TransmitPause = DISABLE;
  hfdcan2.Init.ProtocolException = DISABLE;
  hfdcan2.Init.NominalPrescaler = 30;
  hfdcan2.Init.NominalSyncJumpWidth = 1;
  hfdcan2.Init.NominalTimeSeg1 = 7;
  hfdcan2.Init.NominalTimeSeg2 = 2;
  hfdcan2.Init.DataPrescaler = 1;
  hfdcan2.Init.DataSyncJumpWidth = 1;
  hfdcan2.Init.DataTimeSeg1 = 1;
  hfdcan2.Init.DataTimeSeg2 = 1;
  hfdcan2.Init.MessageRAMOffset = 0;
  hfdcan2.Init.StdFiltersNbr = 0;
  hfdcan2.Init.ExtFiltersNbr = 0;
  hfdcan2.Init.RxFifo0ElmtsNbr = 8;
  hfdcan2.Init.RxFifo0ElmtSize = FDCAN_DATA_BYTES_8;
  hfdcan2.Init.RxFifo1ElmtsNbr = 0;
  hfdcan2.Init.RxFifo1ElmtSize = FDCAN_DATA_BYTES_8;
  hfdcan2.Init.RxBuffersNbr = 0;
  hfdcan2.Init.RxBufferSize = FDCAN_DATA_BYTES_8;
  hfdcan2.Init.TxEventsNbr = 0;
  hfdcan2.Init.TxBuffersNbr = 0;
  hfdcan2.Init.TxFifoQueueElmtsNbr = 8;
  hfdcan2.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;
  hfdcan2.Init.TxElmtSize = FDCAN_DATA_BYTES_8;
  if (HAL_FDCAN_Init(&hfdcan2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN FDCAN2_Init 2 */

  /* USER CODE END FDCAN2_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x009034B6;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief IWDG1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_IWDG1_Init(void)
{

  /* USER CODE BEGIN IWDG1_Init 0 */

  /* USER CODE END IWDG1_Init 0 */

  /* USER CODE BEGIN IWDG1_Init 1 */

  /* USER CODE END IWDG1_Init 1 */
  hiwdg1.Instance = IWDG1;
  hiwdg1.Init.Prescaler = IWDG_PRESCALER_32;
  hiwdg1.Init.Window = 4095;
  hiwdg1.Init.Reload = 499;
  if (HAL_IWDG_Init(&hiwdg1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN IWDG1_Init 2 */

  /* USER CODE END IWDG1_Init 2 */

}

/**
  * @brief LTDC Initialization Function
  * @param None
  * @retval None
  */
static void MX_LTDC_Init(void)
{

  /* USER CODE BEGIN LTDC_Init 0 */
  /* USER CODE END LTDC_Init 0 */

  LTDC_LayerCfgTypeDef pLayerCfg = {0};

  /* USER CODE BEGIN LTDC_Init 1 */
  /* USER CODE END LTDC_Init 1 */
  hltdc.Instance = LTDC;
  hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
  hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
  hltdc.Init.DEPolarity = LTDC_DEPOLARITY_AH;
  hltdc.Init.PCPolarity = LTDC_PCPOLARITY_IPC;
  hltdc.Init.HorizontalSync = 47;
  hltdc.Init.VerticalSync = 2;
  hltdc.Init.AccumulatedHBP = 87;
  hltdc.Init.AccumulatedVBP = 31;
  hltdc.Init.AccumulatedActiveW = 887;
  hltdc.Init.AccumulatedActiveH = 511;
  hltdc.Init.TotalWidth = 927;
  hltdc.Init.TotalHeigh = 524;
  hltdc.Init.Backcolor.Blue = 0;
  hltdc.Init.Backcolor.Green = 0;
  hltdc.Init.Backcolor.Red = 0;
  if (HAL_LTDC_Init(&hltdc) != HAL_OK)
  {
    Error_Handler();
  }
  pLayerCfg.WindowX0 = 0;
  pLayerCfg.WindowX1 = 800;
  pLayerCfg.WindowY0 = 0;
  pLayerCfg.WindowY1 = 480;
  pLayerCfg.PixelFormat = LTDC_PIXEL_FORMAT_RGB565;
  pLayerCfg.Alpha = 255;
  pLayerCfg.Alpha0 = 0;
  pLayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_CA;
  pLayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_CA;
  pLayerCfg.FBStartAdress = 0;
  pLayerCfg.ImageWidth = 800;
  pLayerCfg.ImageHeight = 480;
  pLayerCfg.Backcolor.Blue = 0;
  pLayerCfg.Backcolor.Green = 0;
  pLayerCfg.Backcolor.Red = 0;
  if (HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LTDC_Init 2 */
  /* USER CODE END LTDC_Init 2 */

}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */

  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
  sTime.Hours = 0x0;
  sTime.Minutes = 0x0;
  sTime.Seconds = 0x0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  sDate.WeekDay = RTC_WEEKDAY_MONDAY;
  sDate.Month = RTC_MONTH_JANUARY;
  sDate.Date = 0x1;
  sDate.Year = 0x0;

  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief SPI4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI4_Init(void)
{

  /* USER CODE BEGIN SPI4_Init 0 */

  /* USER CODE END SPI4_Init 0 */

  /* USER CODE BEGIN SPI4_Init 1 */

  /* USER CODE END SPI4_Init 1 */
  /* SPI4 parameter configuration*/
  hspi4.Instance = SPI4;
  hspi4.Init.Mode = SPI_MODE_MASTER;
  hspi4.Init.Direction = SPI_DIRECTION_2LINES;
  hspi4.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi4.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi4.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi4.Init.NSS = SPI_NSS_SOFT;
  hspi4.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi4.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi4.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi4.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi4.Init.CRCPolynomial = 0x0;
  hspi4.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  hspi4.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi4.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi4.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi4.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi4.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi4.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi4.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi4.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi4.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  if (HAL_SPI_Init(&hspi4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI4_Init 2 */

  /* USER CODE END SPI4_Init 2 */

}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{

  /* USER CODE BEGIN TIM4_Init 0 */

  /* USER CODE END TIM4_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 39;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 99;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */
  HAL_TIM_MspPostInit(&htim4);

}

/**
  * @brief TIM7 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM7_Init(void)
{

  /* USER CODE BEGIN TIM7_Init 0 */

  /* USER CODE END TIM7_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM7_Init 1 */

  /* USER CODE END TIM7_Init 1 */
  htim7.Instance = TIM7;
  htim7.Init.Prescaler = 199;
  htim7.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim7.Init.Period = 999;
  htim7.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim7) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim7, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM7_Init 2 */

  /* USER CODE END TIM7_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart2, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart2, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
  /* DMA1_Stream1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);
  /* DMA1_Stream2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream2_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream2_IRQn);
  /* DMA1_Stream3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);
  /* DMA1_Stream4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);
  /* DMA1_Stream5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);

}

/* FMC initialization function */
static void MX_FMC_Init(void)
{

  /* USER CODE BEGIN FMC_Init 0 */

  /* USER CODE END FMC_Init 0 */

  FMC_SDRAM_TimingTypeDef SdramTiming = {0};

  /* USER CODE BEGIN FMC_Init 1 */

  /* USER CODE END FMC_Init 1 */

  /** Perform the SDRAM1 memory initialization sequence
  */
  hsdram1.Instance = FMC_SDRAM_DEVICE;
  /* hsdram1.Init */
  hsdram1.Init.SDBank = FMC_SDRAM_BANK2;
  hsdram1.Init.ColumnBitsNumber = FMC_SDRAM_COLUMN_BITS_NUM_8;
  hsdram1.Init.RowBitsNumber = FMC_SDRAM_ROW_BITS_NUM_12;
  hsdram1.Init.MemoryDataWidth = FMC_SDRAM_MEM_BUS_WIDTH_16;
  hsdram1.Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4;
  hsdram1.Init.CASLatency = FMC_SDRAM_CAS_LATENCY_2;
  hsdram1.Init.WriteProtection = FMC_SDRAM_WRITE_PROTECTION_DISABLE;
  hsdram1.Init.SDClockPeriod = FMC_SDRAM_CLOCK_PERIOD_2;
  hsdram1.Init.ReadBurst = FMC_SDRAM_RBURST_ENABLE;
  hsdram1.Init.ReadPipeDelay = FMC_SDRAM_RPIPE_DELAY_1;
  /* SdramTiming */
  SdramTiming.LoadToActiveDelay = 2;
  SdramTiming.ExitSelfRefreshDelay = 7;
  SdramTiming.SelfRefreshTime = 5;
  SdramTiming.RowCycleDelay = 7;
  SdramTiming.WriteRecoveryTime = 3;
  SdramTiming.RPDelay = 2;
  SdramTiming.RCDDelay = 2;

  if (HAL_SDRAM_Init(&hsdram1, &SdramTiming) != HAL_OK)
  {
    Error_Handler( );
  }

  /* USER CODE BEGIN FMC_Init 2 */
 // FMC_SDRAM_CommandTypeDef command;
   // Program the SDRAM external device
  // BSP_SDRAM_Initialization_Sequence(&hsdram1, &command);
   SDRAM_Initialization_Sequence(&hsdram1);
  /* USER CODE END FMC_Init 2 */
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOI_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOJ_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOK_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(ETH_SCSn_GPIO_Port, ETH_SCSn_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOI, ETH_PMODE0_Pin|ETH_PMODE2_Pin|BUZZER_EN_Pin|BUZZER_MUTE_Pin
                          |ETH_RSTn_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, ETH_PMODE1_Pin|RS422_DE_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LCD_ONOFF_GPIO_Port, LCD_ONOFF_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(RS422_RE_GPIO_Port, RS422_RE_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOG, HC165_EN_Pin|HC165_CLK_Pin|HC165_MOSI_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_DRV_EN_GPIO_Port, LED_DRV_EN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : ETH_INTn_Pin */
  GPIO_InitStruct.Pin = ETH_INTn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(ETH_INTn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : ETH_SCSn_Pin */
  GPIO_InitStruct.Pin = ETH_SCSn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(ETH_SCSn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : ETH_PMODE0_Pin ETH_PMODE2_Pin BUZZER_EN_Pin BUZZER_MUTE_Pin
                           ETH_RSTn_Pin */
  GPIO_InitStruct.Pin = ETH_PMODE0_Pin|ETH_PMODE2_Pin|BUZZER_EN_Pin|BUZZER_MUTE_Pin
                          |ETH_RSTn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);

  /*Configure GPIO pins : ETH_PMODE1_Pin RS422_DE_Pin */
  GPIO_InitStruct.Pin = ETH_PMODE1_Pin|RS422_DE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : LCD_ONOFF_Pin */
  GPIO_InitStruct.Pin = LCD_ONOFF_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LCD_ONOFF_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : RS422_RE_Pin */
  GPIO_InitStruct.Pin = RS422_RE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(RS422_RE_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : HC165_EN_Pin HC165_CLK_Pin HC165_MOSI_Pin */
  GPIO_InitStruct.Pin = HC165_EN_Pin|HC165_CLK_Pin|HC165_MOSI_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /*Configure GPIO pin : HC165_MISO_Pin */
  GPIO_InitStruct.Pin = HC165_MISO_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(HC165_MISO_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LED_DRV_EN_Pin */
  GPIO_InitStruct.Pin = LED_DRV_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_DRV_EN_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
// SDRAM Başlatma Fonksiyonu
void SDRAM_Initialization_Sequence(SDRAM_HandleTypeDef *hsdram)
{
    FMC_SDRAM_CommandTypeDef Command;
    uint32_t tmpmrd = 0;

    /* 1. ADIM: Clock (Saat) Sinyalini Aktif Et ============================ */
    Command.CommandMode            = FMC_SDRAM_CMD_CLK_ENABLE;
    Command.CommandTarget          = FMC_SDRAM_CMD_TARGET_BANK2; // 0xD0000000 Bank 2'dir
    Command.AutoRefreshNumber      = 1;
    Command.ModeRegisterDefinition = 0;

    HAL_SDRAM_SendCommand(hsdram, &Command, HAL_MAX_DELAY);

    /* 2. ADIM: SDRAM'in kendine gelmesi için en az 100us bekle ========== */
    HAL_Delay(1); // 1 ms beklemek garanti çözümdür

    /* 3. ADIM: Tüm Bankları  ?arj Et (Precharge All) ===================== */
    Command.CommandMode            = FMC_SDRAM_CMD_PALL;
    Command.CommandTarget          = FMC_SDRAM_CMD_TARGET_BANK2;
    Command.AutoRefreshNumber      = 1;
    Command.ModeRegisterDefinition = 0;

    HAL_SDRAM_SendCommand(hsdram, &Command, HAL_MAX_DELAY);

    /* 4. ADIM: Auto-Refresh (Otomatik Yenileme) Komutunu Gönder (8 Kez) == */
    Command.CommandMode            = FMC_SDRAM_CMD_AUTOREFRESH_MODE;
    Command.CommandTarget          = FMC_SDRAM_CMD_TARGET_BANK2;
    Command.AutoRefreshNumber      = 8; // Datasheet genelde 2 veya 8 ister, 8 en güvenlisidir
    Command.ModeRegisterDefinition = 0;

    HAL_SDRAM_SendCommand(hsdram, &Command, HAL_MAX_DELAY);

    /* 5. ADIM: Mode Register Ayarlarını Gönder (A3/A5 Ters Döndürme İle!) */
    tmpmrd = (uint32_t)SDRAM_MODEREG_BURST_LENGTH_1          |
                       SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL   |
                       SDRAM_MODEREG_CAS_LATENCY_2           |
                       SDRAM_MODEREG_OPERATING_MODE_STANDARD |
                       SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;

    // Normalde tmpmrd değeri 0x0220'dir. Makromuz bunu 0x0208'e çevirecek.
    // Böylece PCB'deki ters yollar, bu tersliği düzeltecek ve SDRAM'e kusursuz ulaşacak!
    Command.ModeRegisterDefinition = SWAP_A3_A5(tmpmrd);

    Command.CommandMode            = FMC_SDRAM_CMD_LOAD_MODE;
    Command.CommandTarget          = FMC_SDRAM_CMD_TARGET_BANK2;
    Command.AutoRefreshNumber      = 1;

    HAL_SDRAM_SendCommand(hsdram, &Command, HAL_MAX_DELAY);

    /* IS42S16400J: 4096 satir / 64 ms, SDCLK = 100 MHz
         * (64e-3 / 4096) * 100e6 - 20 = 1542 */
    HAL_SDRAM_ProgramRefreshRate(hsdram, 1542);
}

void Set_LCD_Brightness(uint8_t value)
{
    TIM4->CCR1 = value;
}

// DMA GÖNDERİMİ BİTTİ�?İNDE ÇALI�?AN KESME
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	RS422_TxCpltCallback(huart);
	UART2_TxCpltCallback(huart);
}

// IDLE KESMESİ (DMA VERİ ALMAYI BİTİRDİ�?İNDE VEYA PAKET GELDİ�?İNDE ÇALI�?IR)
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
   RS422_Data_Received_Callback(huart, Size);
   UART2_RxEventCallback(huart, Size);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        RS422_ErrorCallback(huart); // RS422 Hatalarını Oraya Gönder
    }
    else if (huart->Instance == USART2)
    {
        UART2_ErrorCallback(huart); // BMB Hatalarını Oraya Gönder
    }
}
void HAL_LTDC_ErrorCallback(LTDC_HandleTypeDef *hltdc_ptr)
{
    if (__HAL_LTDC_GET_FLAG(hltdc_ptr, LTDC_FLAG_FU)) dbg_ltdcUnderrun++;
    if (__HAL_LTDC_GET_FLAG(hltdc_ptr, LTDC_FLAG_TE)) dbg_ltdcXferErr++;
}

void Hardware_Transmit_Data(uint8_t* data, uint16_t length)
{
    // Sistemine göre TX fonksiyonunu buraya yazacaksın.
    // �?imdilik boş bırakabilir veya UDP TX fonksiyonunu ekleyebilirsin:
    // UDP_Send_Data(TARGET_1_SOCKET, data, length, TARGET_1_IP, TARGET_1_PORT);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM7)
    {
        Buzzer_ProcessHandler();   /* artik 1 ms cozunurlukte, jitter yok */
    }
}

// C Tarafı İçin Basit Checksum Hesaplayıcı
uint8_t Calculate_Checksum_C(uint8_t* data, uint16_t len) {
    uint16_t sum = 0;
    for(uint16_t i = 0; i < len; i++) sum += data[i];
    return (uint8_t)(sum & 0xFF);
}

// Decimal'den BCD'ye Çevirici Makro (Saat ve Dakika için)
#define DEC2BCD(val) ((((val) / 10) << 4) | ((val) % 10))

void Debug_Data_Injector(void)
{
    // Sistem çalıştığı an kendi kendine saniyede 25 kere (40ms) veri göndersin
    if (HAL_GetTick() - lattime > 40)
    {
        lattime = HAL_GetTick();



        // Güvenlik: Maksimum 5 hedef ayarladık
        uint8_t count = dbg_threat_count;
        if (count > MAX_DBG_THREATS) count = MAX_DBG_THREATS;

        // Paket uzunluğunu dinamik hesapla
        uint16_t packet_len = 4 + 2 + (count * 14) + 1;
        uint8_t buf[150];

        buf[0] = 0x02; // SRC_TLUS
        buf[1] = 0x01; // MSG_TLUS_TEHDITLERI
        buf[2] = (packet_len & 0xFF);         // LSB
        buf[3] = ((packet_len >> 8) & 0xFF);  // MSB

        buf[4] = 0x80; // Geçerli
        buf[5] = count;

        int offset = 6;

        // Her bir hedef için dizilerdeki (Array) senin ayarlarını okuyup ICD'ye paketleyelim
        for (int i = 0; i < count; i++)
        {
            // Sınıf(15-12), AgeOut(11), Öncelik(10-6)
            uint16_t w7_8 = ((dbg_threat_classes[i] & 0x0F) << 12) |
                            ((dbg_threat_ageout[i] & 0x01) << 11) |
                            ((dbg_threat_prios[i] & 0x1F) << 6);
            buf[offset]   = (w7_8 >> 8) & 0xFF;
            buf[offset+1] = (w7_8 & 0xFF);

            // Tehdit ID (Görsellerin sapıtmaması için her slotun ID'si sabittir: 1,2,3..)
            buf[offset+2] = 0x00;
            buf[offset+3] = i + 1;

            // Band ve Açı
            uint16_t raw_angle = (uint16_t)(dbg_threat_angles[i] * 10); // 30 derece -> 300 olur
            uint16_t w11_12 = ((dbg_threat_bands[i] & 0x0F) << 12) | (raw_angle & 0x0FFF);
            buf[offset+4] = (w11_12 >> 8) & 0xFF;
            buf[offset+5] = (w11_12 & 0xFF);

            // Track Code ve PRF (Boş)
            buf[offset+6]=0; buf[offset+7]=0; buf[offset+8]=0; buf[offset+9]=0;
            buf[offset+10]=0; buf[offset+11]=0; buf[offset+12]=0; buf[offset+13]=0;

            offset += 14;
        }

        // Checksum
        buf[packet_len - 1] = Calculate_Checksum_C(buf, packet_len - 1);

        // TouchGFX Model'e yolla
        TLUS_Donanimdan_Gelen_Veri(buf, packet_len);
    }

    else if (dbg_send_time == 1)
	{
		dbg_send_time = 0;

		uint8_t buf[15];
		buf[0] = 0x02; // SRC_TLUS
		buf[1] = 0x0C; // MSG_TARIH_ZAMAN
		buf[2] = 15;   // Uzunluk (LSB)
		buf[3] = 0x00; // Uzunluk (MSB)

		// Byte 5-6: Geçerlilik (Bit 1-0 = 1, RTC Geçerli)
		buf[4] = 0x00;
		buf[5] = 0x01;

		// Byte 7-8: Saat ve Dakika (BCD)
		buf[6] = DEC2BCD(dbg_time_hour);
		buf[7] = DEC2BCD(dbg_time_minute);

		// Yılın Gününü (Örn: 28 Ağustos = 240. gün) hesaplayalım
		uint16_t dayOfYear = dbg_time_dayOfYear;
		uint8_t yuzler = (dayOfYear / 100) % 10;
		uint8_t onlar  = (dayOfYear / 10) % 10;
		uint8_t birler = dayOfYear % 10;

		// Byte 9-10: Saniye (MSB) ve Yılın Günü Yüzler/Onlar (LSB)
		buf[8] = DEC2BCD(dbg_time_second);
		buf[9] = (yuzler << 4) | (onlar & 0x0F); // BCD Formatında birleştir

		// Byte 11-12: Yılın Günü Birler (MSB) ve Rezerve (LSB)
		buf[10]= (birler << 4) | 0x00; // Alt 4 bit rezerve
		buf[11]= 0x00;

		// Byte 13-14: Yıl (Unsigned short LSB 1)
		buf[12] = (dbg_time_year >> 8) & 0xFF; // MSB
		buf[13] = (dbg_time_year & 0xFF);      // LSB

		// Byte 15: Checksum
		buf[14] = Calculate_Checksum_C(buf, 14);

		TLUS_Donanimdan_Gelen_Veri(buf, 15);
	}
    else if (dbg_send_sys_status > 0)
	{
		uint8_t buf[60] = {0}; // ICD'ye göre Sistem Durum Paketi tam 60 Byte

		buf[0] = 0x02; // SRC_TLUS
		buf[1] = 0x08; // MSG_SISTEM_DURUMU
		buf[2] = 60;   // Uzunluk LSB
		buf[3] = 0x00; // Uzunluk MSB

		// Byte 5-6: Genel Durum ve Geçerlilik (Bit 15 = 1)
		buf[4] = 0x80;
		buf[5] = 0x00;

		// =======================================================
		// İ�?LEMCİ BİRİMİ HATALARI (Byte 6-11)
		// =======================================================
		buf[6]  = (dbg_sys_proc_faults[0] >> 8) & 0xFF;
		buf[7]  = dbg_sys_proc_faults[0] & 0xFF;

		buf[8]  = (dbg_sys_proc_faults[1] >> 8) & 0xFF;
		buf[9]  = dbg_sys_proc_faults[1] & 0xFF;

		buf[10] = (dbg_sys_proc_faults[2] >> 8) & 0xFF;
		buf[11] = dbg_sys_proc_faults[2] & 0xFF;

		// =======================================================
		// SENSÖR BİRİMİ HATALARI (Byte 14-29 Arası)
		// =======================================================
		for(int i = 0; i < 4; i++) {
			int offset = 14 + (i * 4); // Sensör 1: 14, Sensör 2: 18...

			buf[offset]   = (dbg_sys_sens_faults_msb[i] >> 8) & 0xFF;
			buf[offset+1] = dbg_sys_sens_faults_msb[i] & 0xFF;

			buf[offset+2] = (dbg_sys_sens_faults_lsb[i] >> 8) & 0xFF;
			buf[offset+3] = dbg_sys_sens_faults_lsb[i] & 0xFF;
		}

		// Byte 60: Checksum
		buf[59] = Calculate_Checksum_C(buf, 59);

		// Paketi Fırlat
		TLUS_Donanimdan_Gelen_Veri(buf, 60);

		dbg_send_sys_status = 0; // Tek seferlik gönderim
	}

}
/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0xD0000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_8MB;
  MPU_InitStruct.SubRegionDisable = 0x0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER1;
  MPU_InitStruct.BaseAddress = 0x30000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_128KB;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
	g_fault_marker = 0xDEADBEEFU;
	    /* __disable_irq() YOK - watchdog'un calismasina izin ver */
  /* User can add his own implementation to report the HAL error return state */
 // __disable_irq();
  while (1)
  {
	  /* IWDG beslenmiyor -> ~500 ms icinde reset */
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
