/*
 * LP5036R_LED_DRIVER.c
 *
 *  Created on: Jul 1, 2026
 *      Author: HUSEYIN
 */

#include "lp5036_led_driver.h"
#include "main.h"

#define LP5036_I2C_TIMEOUT   100U

static I2C_HandleTypeDef *LP5036_hi2cx;

/* ==================================================================== */
/* LED PARLAKLIK VE DURUM HAFIZASI                                      */
/* ==================================================================== */
/* Global parlaklık seviyesi: 0-5 arası. Varsayılan: 5 (Maksimum %100)  */
static uint8_t LED_Global_Brightness = 3;

/* Mevcut LED durumlarını hafızada tutmak için (Parlaklık değiştiğinde anında uygulamak için) */
static LP5036_LedState_t LED_Current_States[LP5036_PHYSICAL_LED_COUNT] = { LP5036_LED_OFF };

/**
 * @brief  Global kanal numarasını (0-71) fiziksel cihaz adresine ve
 *         o cihazdaki lokal kanal indeksine (0-35) çevirir.
 *         Bu fonksiyon TEK sorumluluk taşır: "kaçıncı entegre, kaçıncı kanal" sorusunu cevaplar.
 *         İleride cihaz sayısı/sırası değişirse SADECE burası güncellenir.
 *
 * @param  global_channel : 0-71 arası global kanal numarası
 * @param  map            : Sonucun yazılacağı çıktı struct'ı
 * @retval LP5036_OK veya LP5036_ERROR_INVALID_CHANNEL
 */
static LP5036_Status_t LP5036_ResolveChannel(uint8_t global_channel, LP5036_ChannelMap_t *map)
{
    if (global_channel >= LP5036_TOTAL_CHANNELS)
    {
        return LP5036_ERROR_INVALID_CHANNEL;
    }

    uint8_t device_index = global_channel / LP5036_CHANNELS_PER_DEVICE; /* 0 veya 1 */
    map->local_index = global_channel % LP5036_CHANNELS_PER_DEVICE;    /* 0-35 */

    map->dev_addr = (device_index == 0U) ? LP5036_DEV1_ADDR_8BIT : LP5036_DEV2_ADDR_8BIT;

    return LP5036_OK;
}

/*
 * buraya genel sistem hato kodu enumını ekle void olmasın CIT için
 */
void LP5036LedDriver_Init(I2C_HandleTypeDef *HI2Cx)
{
	LP5036_Status_t  LedDriverInitStatus = LP5036_OK;
	LP5036_hi2cx = HI2Cx;

	HAL_GPIO_WritePin(LED_DRV_EN_GPIO_Port, LED_DRV_EN_Pin, GPIO_PIN_RESET);
	HAL_Delay(5);
	HAL_GPIO_WritePin(LED_DRV_EN_GPIO_Port, LED_DRV_EN_Pin, GPIO_PIN_SET);
	HAL_Delay(5);
	LedDriverInitStatus = LP5036_Config();

}

/**
 * @brief  Tek bir register'a tek byte yazar.
 * @param  dev_addr : Cihazın 8-bit I2C adresi
 * @param  reg_addr : Yazılacak register adresi
 * @param  value    : Yazılacak değer
 * @retval LP5036_OK veya LP5036_ERROR_I2C_FAIL
 */
LP5036_Status_t LP5036_WriteReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t value)
{
    HAL_StatusTypeDef hal_status;

    hal_status = HAL_I2C_Mem_Write(LP5036_hi2cx, dev_addr, reg_addr,
                                    I2C_MEMADD_SIZE_8BIT, &value, 1U, LP5036_I2C_TIMEOUT);

    return (hal_status == HAL_OK) ? LP5036_OK : LP5036_ERROR_I2C_FAIL;
}

/**
 * @brief  Ardışık birden fazla register'a auto-increment kullanarak yazar.
 *         (Örn: 36 kanalın tamamını tek çağrıda yazmak için)
 * @param  dev_addr : Cihazın 8-bit I2C adresi
 * @param  reg_addr : Yazmaya başlanacak ilk register adresi
 * @param  data     : Yazılacak byte dizisi
 * @param  len      : Yazılacak byte sayısı
 * @retval LP5036_OK veya LP5036_ERROR_I2C_FAIL
 */
LP5036_Status_t LP5036_WriteRegs(uint8_t dev_addr, uint8_t reg_addr, const uint8_t *data, uint16_t len)
{
    HAL_StatusTypeDef hal_status;

    hal_status = HAL_I2C_Mem_Write(LP5036_hi2cx, dev_addr, reg_addr,
                                    I2C_MEMADD_SIZE_8BIT, (uint8_t *)data, len, LP5036_I2C_TIMEOUT);

    return (hal_status == HAL_OK) ? LP5036_OK : LP5036_ERROR_I2C_FAIL;
}

/**
 * @brief  Tek bir register'ı okur.
 * @param  dev_addr : Cihazın 8-bit I2C adresi
 * @param  reg_addr : Okunacak register adresi
 * @param  value    : Okunan değerin yazılacağı adres
 * @retval LP5036_OK veya LP5036_ERROR_I2C_FAIL
 */
LP5036_Status_t LP5036_ReadReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *value)
{
    HAL_StatusTypeDef hal_status;

    hal_status = HAL_I2C_Mem_Read(LP5036_hi2cx, dev_addr, reg_addr,
                                   I2C_MEMADD_SIZE_8BIT, value, 1U, LP5036_I2C_TIMEOUT);

    return (hal_status == HAL_OK) ? LP5036_OK : LP5036_ERROR_I2C_FAIL;
}


/**
 * @brief  Tek bir LP5036 cihazını NORMAL moda alır ve varsayılan
 *         konfigürasyonu (auto-increment, dithering vb.) uygular.
 *         Datasheet 8.4 Device Functional Modes akışına göre:
 *         Chip_EN=1 yapılmadan cihaz NORMAL moda geçmez, LED çıkışları çalışmaz.
 *
 * @param  dev_addr : Konfigüre edilecek cihazın 8-bit I2C adresi
 * @retval LP5036_OK veya LP5036_ERROR_I2C_FAIL
 */
LP5036_Status_t LP5036_DeviceConfig(uint8_t dev_addr)
{
    LP5036_Status_t status;

    /* 1) Chip_EN = 1 -> cihazı NORMAL moda al (DEVICE_CONFIG0, bit6) */
    status = LP5036_WriteReg(dev_addr, LP5036_REG_DEVICE_CONFIG0, LP5036_CHIP_EN);
    if (status != LP5036_OK)
    {
        return status;
    }

    /* 2) DEVICE_CONFIG1: auto-increment ve dithering acik, logaritmik skala,
     *    power-save acik, max akim secenegi varsayilan (25.5mA) birakildi.
     *    Not: bu deger reset default'u (0x3C) ile ayni, yine de acikca yaziliyor. */
    uint8_t config1 = LP5036_LOG_SCALE_EN
                     | LP5036_POWER_SAVE_EN
                     | LP5036_AUTO_INCR_EN
                     | LP5036_PWM_DITHERING_EN; // | LP5036_LED_GLOBAL_OFF  | LP5036_MAX_CURRENT_OPTION

    status = LP5036_WriteReg(dev_addr, LP5036_REG_DEVICE_CONFIG1, config1);
    if (status != LP5036_OK)
    {
        return status;
    }

    return LP5036_OK;
}

/**
 * @brief  Sistemdeki TUM LP5036 cihazlarini (DEV1 ve DEV2) sirayla konfigure eder.
 *         Uygulama kodunun cagirmasi gereken tek konfigurasyon fonksiyonu budur.
 *
 * @retval LP5036_OK veya ilk basarisiz olan cihazin hata kodu
 */
LP5036_Status_t LP5036_Config(void)
{
    LP5036_Status_t status;

    status = LP5036_DeviceConfig(LP5036_DEV1_ADDR_8BIT);
    if (status != LP5036_OK)
    {
        return status;
    }

    status = LP5036_DeviceConfig(LP5036_DEV2_ADDR_8BIT);
    if (status != LP5036_OK)
    {
        return status;
    }

    return LP5036_OK;
}

/**
 * @brief  Global kanal numarasindaki (0-71) LED cikisinin rengini (OUTx_COLOR) ayarlar.
 *         Hangi fiziksel cihaza ve hangi lokal kanala denk geldigini kendi icinde
 *         cozer, cagiran taraf sadece tek bir entegre varmis gibi kullanir.
 *
 * @param  global_channel : 0-71 arasi global kanal numarasi
 * @param  color_value    : 0x00-0xFF arasi renk karistirma yuzdesi (OUTx_COLOR register degeri)
 * @retval LP5036_OK, LP5036_ERROR_INVALID_CHANNEL veya LP5036_ERROR_I2C_FAIL
 */
LP5036_Status_t LP5036_SetColor(uint8_t global_channel, uint8_t color_value)
{
    LP5036_ChannelMap_t map;
    LP5036_Status_t status;

    status = LP5036_ResolveChannel(global_channel, &map);
    if (status != LP5036_OK)
    {
        return status;
    }

    uint8_t reg_addr = LP5036_REG_OUT0_COLOR + map.local_index;

    status = LP5036_WriteReg(map.dev_addr, reg_addr, color_value);

    return status;
}


/**
 * @brief  Fiziksel LED numarasi -> R ve G bacaklarinin global kanal numaralari.
 *         Kablolama duzensiz oldugu icin her satir gercek devre semasina gore
 *         ELLE doldurulmalidir. Kodun geri kalani bu tablodan tamamen habersizdir.
 *
 *         ASAGIDAKI DEGERLER YER TUTUCUDUR, GERCEK KABLOLAMAYA GORE DOLDURULMALI.
 */
static const LP5036_LedPinMap_t LP5036_LedMap[LP5036_PHYSICAL_LED_COUNT] =
{

	/* LED0 */  	{ .r_channel = 23,  .g_channel = 22  },	//led27 R G  /sis1_G sis1_R
	/* LED1 */  	{ .r_channel = 19,  .g_channel = 18  },	//led3  R G  /sis3_G sis3_R
	/* LED2 */  	{ .r_channel = 15,  .g_channel = 14  },	//led7  R G  /sis5_G sis5_R
	/* LED3 */  	{ .r_channel = 11,  .g_channel = 10  },	//led19 R G	 /sis7_G sis7_R

	/* LED4 */  	{ .r_channel = 21,  .g_channel = 20  },	//led26 R G  /sis2_G sis2_R
	/* LED5 */  	{ .r_channel = 17,  .g_channel = 16  },	//led18 R G  /sis4_G sis4_R
	/* LED6 */  	{ .r_channel = 13,  .g_channel = 12  },	//led14 R G  /sis6_G sis6_R
	/* LED7 */  	{ .r_channel = 9,  .g_channel = 8  },	//led20 R G  /sis8_G sis8_R

	/* LED8 */  	{ .r_channel = 67,  .g_channel = 66  },	//led13 R G  /sis9_G   sis_R
	/* LED9 */  	{ .r_channel = 65,  .g_channel = 64  },	//led5  R G  /sis11_G  sis11_R
	/* LED10 */  	{ .r_channel = 61,  .g_channel = 60  },	//led4  R G  /sis13_G  sis13_R
	/* LED11 */  	{ .r_channel = 53,  .g_channel = 52  },	//led10 R G  /sis15_G  sis15_R

	/* LED12 */  	{ .r_channel = 69,  .g_channel = 68  },	//led6  R G  /sis10_G  sis10_R
	/* LED13 */  	{ .r_channel = 63,  .g_channel = 62  },	//led12 R G  /sis12_G  sis12_R
	/* LED14 */  	{ .r_channel = 59,  .g_channel = 58  },	//led11 R G  /sis14_G  sis14_R
	/* LED15 */  	{ .r_channel = 57,  .g_channel = 54  },	//led23 R G  /sis16_G  sis16_R

	/* LED16 */  	{ .r_channel = 44,  .g_channel = 45  },	//led17 R G  /kom1Up_R  kom1Up_G
	/* LED17 */  	{ .r_channel = 47,  .g_channel = 46  },	//led16 R G  /kom1Dwn_G kom1Dwn_R
	/* LED18 */  	{ .r_channel = 48,  .g_channel = 49  },	//led25 R G  /kom2Dwn_R kom2Dwn_G
	/* LED19 */  	{ .r_channel = 50,  .g_channel = 51  },	//led24 R G  /kom2Up_R  kom2Up_G


    /* LED20 */  	{ .r_channel = 4,  .g_channel = 5  },	//btn   btn  /buton 3
	/* LED21 */  	{ .r_channel = 6,  .g_channel = 7  },	//btn   btn	 /buton 4
	/* LED22 */  	{ .r_channel = 24,  .g_channel = 25  },	//btn   btn  /button 2
	/* LED23 */  	{ .r_channel = 26,  .g_channel = 27  },	//btn   btn  /button 1
	/* LED24 */  	{ .r_channel = 55,  .g_channel = 56  },	//btn   btn  /button 5
	/* LED25 */  	{ .r_channel = 28,  .g_channel = 35  },	//btn   btn  /buton sağ 1
	/* LED26 */  	{ .r_channel = 30,  .g_channel = 33  },	//btn   btn  /buton sağ 3
	/* LED27 */  	{ .r_channel = 32,  .g_channel = 31  },	//btn   btn  /buton sağ 4
	/* LED28 */  	{ .r_channel = 34,  .g_channel = 29  },	//btn   btn  /buton sağ 2
	/* LED29 */  	{ .r_channel = 36,  .g_channel = 43  },	//btn   btn  /buton sol 1
	/* LED30 */  	{ .r_channel = 38,  .g_channel = 41  },	//btn   btn  /buton sol 3
	/* LED31 */  	{ .r_channel = 40,  .g_channel = 39  },	//btn   btn  /buton sol 4
	/* LED32 */  	{ .r_channel = 42,  .g_channel = 37  },	//btn   btn  /buton sol 2

	/* LED33 */  	{ .r_channel = 3,  .g_channel = 2  },   //on    boş  /on_off_led

	/* LED34 */  	{ .r_channel = 0,  .g_channel = 1  },	//boş   boş  /boş

};

/**
 * @brief  Belirtilen fiziksel LED'i istenen duruma getirir (OFF/RED/GREEN/DIM).
 *         Cagiran taraf hangi kanalin R hangisinin G oldugunu bilmek zorunda degildir,
 *         sadece LED indeksini ve istedigi durumu verir.
 *
 * @param  led_index : 0 - (LP5036_PHYSICAL_LED_COUNT-1) arasi fiziksel LED numarasi
 * @param  state     : LP5036_LED_OFF / RED / GREEN / DIM / vb.
 * @retval LP5036_OK, LP5036_ERROR_INVALID_CHANNEL veya LP5036_ERROR_I2C_FAIL
 */
LP5036_Status_t LP5036_SetLedState(uint8_t led_index, LP5036_LedState_t state)
{
    if (led_index >= LP5036_PHYSICAL_LED_COUNT)
    {
        return LP5036_ERROR_INVALID_CHANNEL;
    }

    // 1. Yeni durumu hafızaya kaydet (Parlaklık değiştiğinde hatırlamak için)
    LED_Current_States[led_index] = state;

    LP5036_LedPinMap_t pins = LP5036_LedMap[led_index];
    uint32_t r_base = 0; // Matematiksel taşmaları önlemek için 32-bit tanımlıyoruz
    uint32_t g_base = 0;

    // 2. Renklerin %100 (Ham) Değerlerini Belirle
    switch (state)
    {
        case LP5036_LED_RED:
            r_base = LP5036_RED_LEVEL;
            g_base = 0x00U;
            break;

        case LP5036_LED_GREEN:
            r_base = 0x00U;
            g_base = LP5036_GREEN_LEVEL;
            break;

        case LP5036_LED_BOTH:
            r_base = 0xFFU;
            g_base = 0xFFU;
            break;

        case LP5036_LED_DIM:
            r_base = LP5036_DIM_LEVEL;
            g_base = LP5036_DIM_LEVEL;
            break;

        case LP5036_LED_ORANGE:
            // İşlem önceliği ve taşma riskine karşı parantez ve çarpma işlemi düzeltildi
            r_base = (LP5036_RED_LEVEL * 5) / 6;
            g_base = (LP5036_GREEN_LEVEL * 5) / 6;
            break;

        case LP5036_LED_OFF:
        default:
            r_base = 0x00U;
            g_base = 0x00U;
            break;
    }

    // 3. Global Parlaklık Çarpanını Uygula (0 - 5 Arası)
    // Formül: (Ham_Deger * Seviye) / 5
    uint8_t r_value = (uint8_t)((r_base * LED_Global_Brightness) / 5);
    uint8_t g_value = (uint8_t)((g_base * LED_Global_Brightness) / 5);

    LP5036_Status_t status;

    // 4. Hesaplanmış yeni değerleri donanıma yaz
    status = LP5036_SetColor(pins.r_channel, r_value);
    if (status != LP5036_OK)
    {
        return status;
    }

    status = LP5036_SetColor(pins.g_channel, g_value);

    return status;
}

/**
 * @brief  Yardimci fonksiyon: DEVICE_CONFIG1 register'inin LED_GLOBAL_OFF
 *         bitini istenen deger ile ayarlar (diger bitlere dokunmadan).
 *
 * @param  dev_addr : Cihazin 8-bit I2C adresi
 * @param  turn_off : 1 ise tum LED'leri kapat, 0 ise normal calismaya donder
 * @retval LP5036_OK, LP5036_ERROR_I2C_FAIL
 */
static LP5036_Status_t LP5036_SetGlobalOffBit(uint8_t dev_addr, uint8_t turn_off)
{
    LP5036_Status_t status;
    uint8_t current_config1;

    status = LP5036_ReadReg(dev_addr, LP5036_REG_DEVICE_CONFIG1, &current_config1);
    if (status != LP5036_OK)
    {
        return status;
    }

    if (turn_off)
    {
        current_config1 |= LP5036_LED_GLOBAL_OFF;
    }
    else
    {
        current_config1 &= (uint8_t)~LP5036_LED_GLOBAL_OFF;
    }

    return LP5036_WriteReg(dev_addr, LP5036_REG_DEVICE_CONFIG1, current_config1);
}

/**
 * @brief  Sistemdeki TUM cihazlardaki TUM LED cikislarini tek seferde kapatir.
 *         Kanal kanal donguye girmez, DEVICE_CONFIG1'in LED_GLOBAL_OFF bitini
 *         kullanarak tek I2C islemiyle her cihazi soundurur (verimli yontem).
 *
 * @retval LP5036_OK veya ilk basarisiz olan cihazin hata kodu
 */
LP5036_Status_t LP5036_AllLedsOff(void)
{
    LP5036_Status_t status;

    status = LP5036_SetGlobalOffBit(LP5036_DEV1_ADDR_8BIT, 1U);
    if (status != LP5036_OK)
    {
        return status;
    }

    status = LP5036_SetGlobalOffBit(LP5036_DEV2_ADDR_8BIT, 1U);

    return status;
}

/**
 * @brief  LP5036_AllLedsOff ile kapatilan LED_GLOBAL_OFF bitini kaldirip
 *         cihazlari normal calisma durumuna dondurur.
 *         Not: Bu fonksiyon LED'leri otomatik olarak eski renklerine getirmez,
 *         sadece cikislarin tekrar aktif olmasini saglar. OUTx_COLOR register'lari
 *         kapatma sirasinda degismedigi icin, kapatmadan onceki degerler korunur.
 *
 * @retval LP5036_OK veya ilk basarisiz olan cihazin hata kodu
 */
LP5036_Status_t LP5036_AllLedsOn(void)
{
    LP5036_Status_t status;

    status = LP5036_SetGlobalOffBit(LP5036_DEV1_ADDR_8BIT, 0U);
    if (status != LP5036_OK)
    {
        return status;
    }

    status = LP5036_SetGlobalOffBit(LP5036_DEV2_ADDR_8BIT, 0U);

    return status;
}

/**
 * @brief  Sistemdeki TUM fiziksel LED'leri karartma (DIM) moduna alir.
 *         LED_GLOBAL_OFF gibi donanimsal tek bitlik bir mekanizma bulunmadigi
 *         icin (dim seviyesi OUTx_COLOR uzerinden yazilan yazilimsal bir deger
 *         oldugundan), her fiziksel LED tek tek gezilerek ayarlanir.
 *
 * @retval LP5036_OK veya ilk basarisiz olan LED'in hata kodu
 */
LP5036_Status_t LP5036_AllLedsDim(void)
{
    LP5036_Status_t status;

    for (uint8_t led_index = 0U; led_index < LP5036_PHYSICAL_LED_COUNT; led_index++)
    {
        status = LP5036_SetLedState(led_index, LP5036_LED_DIM);
        if (status != LP5036_OK)
        {
            return status;
        }
    }

    return LP5036_OK;
}

/**
 * @brief  Tüm fiziksel LED'lerin parlaklığını 5 kademeli olarak ayarlar.
 *         Değişiklik, o an yanmakta olan tüm LED'lere anında yansıtılır.
 *
 * @param  level : 0 ile 5 arası parlaklık seviyesi (0 = Kapalı, 5 = %100 Maksimum)
 * @retval LP5036_OK
 */
LP5036_Status_t LP5036_SetGlobalBrightness(uint8_t level)
{
    // Sınır koruması: Girilen değer 5'ten büyükse 5'e eşitle
    if (level > 5)
    {
        level = 5;
    }

    // 1. Yeni seviyeyi global değişkene kaydet
    LED_Global_Brightness = level;

    // 2. Hafızadaki son durumlara göre tüm LED'leri yeni parlaklıkta tekrar sür
    for (uint8_t i = 0; i < LP5036_PHYSICAL_LED_COUNT; i++)
    {
        // İçerideki SetLedState, güncel LED_Global_Brightness çarpanını kullanarak I2C'ye yazacaktır
    	if(i!=33)
    		LP5036_SetLedState(i, LED_Current_States[i]);
    }

    return LP5036_OK;
}

LP5036_Status_t LP5036_SetButtonLeds(LP5036_LedState_t ledState)
{
	LP5036_Status_t LP5036Status;

	for(uint8_t i=20; i<33; i++)
	{
		LP5036Status = LP5036_SetLedState(i, ledState);
		if(LP5036Status != LP5036_OK)
			return LP5036Status;
	}

	return LP5036_OK;
}
